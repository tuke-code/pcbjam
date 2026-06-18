#!/usr/bin/env node
// Publish the static demo "gallery" (example projects the no-backend standalone
// opens read-only; Save downloads to local) to the CDN under content/<tag>/.
// Implements the P3 part of docs/features/demo-deploy/.
//
//   node scripts/publish-content.mjs --tag 2.7.7 --gallery content/gallery.json \
//        --driver local --out /tmp/cdn
//   node scripts/publish-content.mjs --tag 2.7.7 --driver r2 --bucket pcbjam-cdn --remote
//
// `content/gallery.json` CURATES the gallery by REFERENCING source files in the
// repo (so we don't duplicate GPL KiCad data into the closed tree):
//   { "projects": [ { "slug","name","description","root","files":[...] } ] }
// Each content/<tag>/ snapshot is immutable; the app pins
// content/<tag>/manifest.json at build time (VITE_PROJECT_MANIFEST_URL).

import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";
import {
  contentTypeForPath,
  IMMUTABLE,
  makeStore,
  putJSON,
} from "./lib/cdn-store.mjs";

function parseArgs(argv) {
  const a = {
    tag: null,
    gallery: "deploy/demo/gallery.json",
    driver: "local",
    out: null,
    bucket: "pcbjam-cdn",
    remote: false,
    prefix: "content",
    builtAt: process.env.SOURCE_DATE || null,
  };
  for (let i = 2; i < argv.length; i++) {
    const next = () => argv[++i];
    switch (argv[i]) {
      case "--tag": a.tag = next(); break;
      case "--gallery": a.gallery = next(); break;
      case "--driver": a.driver = next(); break;
      case "--out": a.out = next(); break;
      case "--bucket": a.bucket = next(); break;
      case "--remote": a.remote = true; break;
      case "--prefix": a.prefix = next(); break;
      default: throw new Error(`unknown arg: ${argv[i]}`);
    }
  }
  if (!a.tag) throw new Error("--tag <release tag> is required");
  if (a.driver === "local" && !a.out) a.out = ".cdn-out";
  return a;
}

function main() {
  const a = parseArgs(process.argv);
  const builtAt = a.builtAt || new Date().toISOString();
  const store = makeStore(a.driver, a);
  const P = a.prefix;

  const gallery = JSON.parse(readFileSync(a.gallery, "utf8"));
  console.log(
    `publish-content: tag=${a.tag} gallery=${a.gallery} driver=${store.kind} ` +
      `projects=${gallery.projects?.length ?? 0}`,
  );

  const manifest = { schema: 1, tag: a.tag, builtAt, projects: [] };

  for (const proj of gallery.projects ?? []) {
    if (!/^[a-z0-9][a-z0-9._-]*$/.test(proj.slug))
      throw new Error(`invalid project slug: ${proj.slug}`);
    const files = [];
    for (const rel of proj.files) {
      const srcPath = join(proj.root, rel);
      if (!existsSync(srcPath)) throw new Error(`missing source file: ${srcPath}`);
      const bytes = readFileSync(srcPath);
      // Files are served verbatim — the editor fetch()es the raw bytes. Text
      // KiCad files compress fine at the edge; we don't pre-encode them.
      store.put(`${P}/${a.tag}/${proj.slug}/${rel}`, bytes, {
        contentType: contentTypeForPath(rel),
        contentEncoding: null,
        cacheControl: IMMUTABLE,
      });
      files.push({ path: rel, size: bytes.length });
    }
    manifest.projects.push({
      slug: proj.slug,
      name: proj.name ?? proj.slug,
      description: proj.description ?? "",
      files,
    });
    console.log(`  ${proj.slug}: ${files.length} file(s)`);
  }

  // The snapshot is immutable; the app pins this exact URL at build time.
  putJSON(store, `${P}/${a.tag}/manifest.json`, manifest, IMMUTABLE);
  console.log(`done → ${P}/${a.tag}/manifest.json`);
  if (store.kind === "local") console.log(`local layout under: ${a.out}`);
}

main();
