import { SyncStack, type SyncStackOptions } from "@pcbjam/sync-client";

/**
 * Read-only source of 3D model bodies (`.wrl` / `.step`), addressed by the
 * KiCad-relative ref `"<lib>.3dshapes/<name>.<ext>"` (a footprint's `(model …)`
 * path with the `${KICAD*_3DMODEL_DIR}/` prefix stripped).
 *
 * Unlike symbols/footprints, models are never bulk-synced: each lib is an
 * r2-idb-sync **sparse** layer — only the (small) manifest syncs eagerly, and a
 * body is fetched exactly when a board references it, then cached in IDB. This
 * keeps the client cost proportional to what the user renders, not to the
 * ~GB-scale full set (docs/features/3d-models).
 */
export interface Model3dSource {
  /** One model body, IDB-cached; null when unknown/unavailable. */
  getModelBody(ref: string): Promise<Uint8Array | null>;
  /** Whether a ref exists in the set at all (manifest-only, no body fetch). */
  hasModel(ref: string): Promise<boolean>;
}

interface CdnModelsManifest {
  schema: number;
  tag: string;
  libs: Array<{ id: string; itemCount?: number }>;
}

/** Split "<lib>.3dshapes/<name>" → { lib, path: "model3d/<name>" }. */
function splitRef(ref: string): { lib: string; path: string } | null {
  const i = ref.indexOf(".3dshapes/");
  if (i <= 0) return null;
  const lib = ref.slice(0, i);
  const name = ref.slice(i + ".3dshapes/".length);
  if (!name || name.includes("/")) return null;
  return { lib, path: `model3d/${name}` };
}

/**
 * CDN-backed `Model3dSource`. Layout under the manifest's dir
 * (`<cdn>/libs/kicad-models/<tag>/`, see scripts/deploy/publish-models.ts):
 *   manifest.json        top index { schema, tag, libs:[{id,itemCount}] }
 *   <lib>/manifest       per-lib SyncManifest, entries "model3d/<name>.<ext>"
 * Bodies are content-addressed and shared across tags, one level up:
 *   <cdn>/libs/kicad-models/blobs/sha256/<hash>
 */
export function cdnModelsSource(
  manifestUrl: string,
  opts?: Pick<SyncStackOptions, "fetchImpl" | "storeFactory">,
): Model3dSource {
  const baseDir = manifestUrl.replace(/\/[^/]*$/, ""); // …/libs/kicad-models/<tag>
  const blobsBase = `${baseDir.replace(/\/[^/]*$/, "")}/blobs/sha256`;
  const fetchImpl = opts?.fetchImpl ?? fetch;

  let manifestP: Promise<CdnModelsManifest> | null = null;
  const loadManifest = (): Promise<CdnModelsManifest> => {
    if (!manifestP) {
      // Never cache a rejection (mirrors cdn-source.ts): a transient failure
      // must not poison every later model read.
      manifestP = (async () => {
        const r = await fetchImpl(manifestUrl, { cache: "no-store" });
        if (!r.ok) throw new Error(`cdn models manifest ${r.status}: ${manifestUrl}`);
        return (await r.json()) as CdnModelsManifest;
      })().catch((e) => {
        manifestP = null;
        throw e;
      });
    }
    return manifestP;
  };

  // One lazily-opened sparse stack per lib (IDB store keyed by namespace, so a
  // lib's cached models persist and dedupe across sessions).
  const stacks = new Map<string, Promise<SyncStack | null>>();
  const openStack = (libId: string): Promise<SyncStack | null> => {
    let p = stacks.get(libId);
    if (!p) {
      p = (async () => {
        const m = await loadManifest();
        if (!m.libs.some((l) => l.id === libId)) return null; // unknown lib
        const stack = new SyncStack({
          layers: [
            {
              namespace: `kicad-models:${m.tag}:${libId}`,
              kind: "sparse",
              url: `${baseDir}/${encodeURIComponent(libId)}`,
              bodyUrlTemplate: `${blobsBase}/{hash}`,
            },
          ],
          ...opts,
        });
        await stack.open();
        return stack;
      })().catch((e) => {
        stacks.delete(libId); // a failed open must stay retryable
        throw e;
      });
      stacks.set(libId, p);
    }
    return p;
  };

  return {
    async getModelBody(ref: string): Promise<Uint8Array | null> {
      const split = splitRef(ref);
      if (!split) return null;
      try {
        const stack = await openStack(split.lib);
        return stack ? await stack.read(split.path) : null;
      } catch {
        return null; // missing models render as absent, never break the viewer
      }
    },
    async hasModel(ref: string): Promise<boolean> {
      const split = splitRef(ref);
      if (!split) return false;
      try {
        const stack = await openStack(split.lib);
        if (!stack) return false;
        return (await stack.list()).some((e) => e.path === split.path);
      } catch {
        return false;
      }
    },
  };
}
