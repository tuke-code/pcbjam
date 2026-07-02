// Publish KiCad 3D models (kicad-packages3D) to the CDN as r2-idb-sync SPARSE
// origins: per-lib manifests keyed by the upstream tag, bodies content-addressed
// under a shared blobs/ prefix (deduped across tags — models rarely change).
// The standalone's cdnModelsSource opens each lib as a sparse layer: manifest
// synced eagerly (small), bodies fetched exactly when a board references them.
// See web/standalone/src/wasm/libs/models-source.ts + docs/features/3d-models.
//
//   npx tsx scripts/deploy/publish-models.ts --model-tag 10.0.0 \
//     --models-src <kicad-packages3D checkout> --driver local --out /tmp/cdn-models
//   npx tsx scripts/deploy/publish-models.ts --model-tag 10.0.0 --models-src … \
//     --driver r2 --bucket pcbjam-cdn --remote
//   # dev subset: only a few libs
//   … --libs Resistor_SMD,Capacitor_SMD,Package_QFP
//
// Layout under `<prefix>` (default libs/kicad-models):
//   <tag>/manifest.json        top index { schema, tag, libs:[{id,itemCount,bytes}] }
//   <tag>/<lib>/manifest       per-lib SyncManifest { "model3d/<name>": {hash,size,mtime} }
//   blobs/sha256/<hash>        model bodies (brotli, content-addressed, shared)
//   blobs/registry.json        published-blob index (hash → size) for cheap dedup
//
// Idempotent per tag: if <prefix>/<tag>/manifest.json exists the run SKIPS
// (--force overrides). Blobs are skipped per-hash via the registry.

import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, readdirSync, readFileSync, statSync } from "node:fs";
import { dirname, join } from "node:path";
import type { SyncManifest } from "../../web/pcbjam-shared/src/sync-wire.js";
import {
  compressBytes,
  IMMUTABLE,
  makeStore,
  NO_STORE,
  putJSON,
  sha256hex,
} from "./lib/cdn-store.mjs";

const MODELS_URL = "https://gitlab.com/kicad/libraries/kicad-packages3D.git";

/** Model file extensions we publish (locked: WRL + STEP). */
const MODEL_EXTS = [".wrl", ".step", ".stp"];

interface Args {
  modelTag: string | null;
  modelsSrc: string | null;
  clone: string | null;
  driver: string;
  out: string | null;
  bucket: string;
  remote: boolean;
  prefix: string;
  force: boolean;
  libs: string[] | null;
  quality: number;
  compress: "br" | "none";
}

function parseArgs(argv: string[]): Args {
  const a: Args = {
    modelTag: null,
    modelsSrc: null,
    clone: null,
    driver: "local",
    out: null,
    bucket: "pcbjam-cdn",
    remote: false,
    prefix: "libs/kicad-models",
    force: false,
    libs: null,
    quality: 5,
    compress: "br",
  };
  for (let i = 2; i < argv.length; i++) {
    const next = () => argv[++i]!;
    switch (argv[i]) {
      case "--model-tag": a.modelTag = next(); break;
      case "--models-src": a.modelsSrc = next(); break;
      // Clone kicad-packages3D at --model-tag into <dir>/kicad-packages3D
      // (shallow; NOTE: multi-GB working tree) and use it as the source.
      case "--clone": a.clone = next(); break;
      case "--driver": a.driver = next(); break;
      case "--out": a.out = next(); break;
      case "--bucket": a.bucket = next(); break;
      case "--remote": a.remote = true; break;
      case "--prefix": a.prefix = next(); break;
      case "--force": a.force = true; break;
      // Dev subset: publish only these libs (names without .3dshapes).
      case "--libs": a.libs = next().split(",").map((s) => s.trim()).filter(Boolean); break;
      // Brotli quality for bodies (WRL/STEP are text-ish; 5 ≈ 4-5x, fast).
      case "--quality": a.quality = Number(next()); break;
      // "none" for --driver local when a plain static server (e.g. the vite dev
      // server) will serve the blobs — it can't send Content-Encoding: br.
      case "--compress": a.compress = next() as "br" | "none"; break;
      default: throw new Error(`unknown arg: ${argv[i]}`);
    }
  }
  if (!a.modelTag) throw new Error("--model-tag <kicad-packages3D tag> is required");
  if (!a.modelsSrc && !a.clone)
    throw new Error("need --models-src <kicad-packages3D checkout> or --clone <dir>");
  if (a.driver === "local" && !a.out) a.out = ".cdn-models-out";
  return a;
}

function cloneShallow(url: string, dest: string, ref: string): void {
  if (existsSync(dest)) {
    console.log(`clone: ${dest} present — reusing`);
    return;
  }
  mkdirSync(dirname(dest), { recursive: true });
  console.log(`clone: ${url} @ ${ref} → ${dest} (multi-GB — this takes a while)`);
  execFileSync("git", ["clone", "--depth", "1", "--branch", ref, url, dest], {
    stdio: "inherit",
  });
}

/** All `<lib>.3dshapes` dirs under the checkout root (non-recursive: the repo is flat). */
function listModelLibs(src: string): Array<{ id: string; dir: string }> {
  return readdirSync(src, { withFileTypes: true })
    .filter((d) => d.isDirectory() && d.name.endsWith(".3dshapes"))
    .map((d) => ({ id: d.name.slice(0, -".3dshapes".length), dir: join(src, d.name) }))
    .sort((x, y) => x.id.localeCompare(y.id));
}

async function main(): Promise<void> {
  const a = parseArgs(process.argv);
  const store = makeStore(a.driver, a);
  const topKey = `${a.prefix}/${a.modelTag}/manifest.json`;
  const registryKey = `${a.prefix}/blobs/registry.json`;

  if (!a.force && store.getJSON(topKey)) {
    console.log(`publish-models: ${topKey} already published — skipping (use --force)`);
    return;
  }

  if (a.clone) {
    const dest = join(a.clone, "kicad-packages3D");
    cloneShallow(MODELS_URL, dest, a.modelTag!);
    a.modelsSrc ??= dest;
  }

  console.log(
    `publish-models: tag=${a.modelTag} driver=${store.kind} → ${a.prefix}/${a.modelTag}/`,
  );

  // Published-blob index: hash → original size. One GET up front, one PUT at the
  // end — the per-blob "does it exist" probe would otherwise be an R2 round-trip
  // per model (tens of thousands).
  const registry: Record<string, number> =
    (store.getJSON(registryKey) as Record<string, number> | null) ?? {};
  let blobsPut = 0;
  let blobsSkipped = 0;

  let libs = listModelLibs(a.modelsSrc!);
  if (a.libs) {
    const want = new Set(a.libs);
    libs = libs.filter((l) => want.has(l.id));
    const missing = a.libs.filter((id) => !libs.some((l) => l.id === id));
    if (missing.length) console.warn(`publish-models: libs not found: ${missing.join(", ")}`);
  }
  if (!libs.length) throw new Error(`no .3dshapes libs under ${a.modelsSrc}`);

  const topLibs: Array<{ id: string; itemCount: number; bytes: number }> = [];
  let totalItems = 0;
  let totalBytes = 0;

  for (const lib of libs) {
    const files = readdirSync(lib.dir)
      .filter((f) => MODEL_EXTS.some((ext) => f.toLowerCase().endsWith(ext)))
      .sort();
    if (!files.length) continue;

    const entries: SyncManifest["entries"] = {};
    let libBytes = 0;
    for (const f of files) {
      const p = join(lib.dir, f);
      if (!statSync(p).isFile()) continue;
      const body = readFileSync(p);
      const hash = sha256hex(body);
      entries[`model3d/${f}`] = { hash, size: body.length, mtime: 0 };
      libBytes += body.length;

      if (registry[hash] === undefined) {
        // WRL and STEP are text formats — brotli gets ~4-5x. The browser fetch
        // transparently decodes, so IDB caches (and hashes refer to) the
        // ORIGINAL bytes; `no-transform` keeps the edge from re-encoding.
        const { bytes, encoding } = compressBytes(body, a.compress, a.quality);
        store.put(`${a.prefix}/blobs/sha256/${hash}`, bytes, {
          contentType: "application/octet-stream",
          contentEncoding: encoding,
          cacheControl: IMMUTABLE,
        });
        registry[hash] = body.length;
        blobsPut++;
      } else {
        blobsSkipped++;
      }
    }

    const manifest: SyncManifest = { version: 1, entries };
    putJSON(store, `${a.prefix}/${a.modelTag}/${lib.id}/manifest`, manifest, IMMUTABLE);
    topLibs.push({ id: lib.id, itemCount: files.length, bytes: libBytes });
    totalItems += files.length;
    totalBytes += libBytes;
    console.log(`  ${lib.id}: ${files.length} models (${(libBytes / 1e6).toFixed(1)} MB)`);
  }

  putJSON(store, registryKey, registry, NO_STORE);
  putJSON(store, topKey, { schema: 1, tag: a.modelTag, libs: topLibs }, IMMUTABLE);

  console.log(
    `publish-models: done — ${topLibs.length} libs, ${totalItems} models ` +
      `(${(totalBytes / 1e6).toFixed(0)} MB raw), blobs put=${blobsPut} deduped=${blobsSkipped} → ${topKey}`,
  );
  if (store.kind === "local") console.log(`local layout under: ${a.out}`);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
