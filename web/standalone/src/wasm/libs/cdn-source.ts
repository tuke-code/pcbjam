import { SyncStack, type SyncStackOptions } from "@pcbjam/sync-client";
import type { LibInfo, LibItemInfo, LibsSource } from "./source";

/**
 * A read-only, no-backend `LibsSource` that serves the FULL default KiCad symbol
 * + footprint set from immutable CDN snapshots (the demo's libs). Each lib is a
 * version-pinned r2-idb-sync **static origin** (docs/features/r2-idb-sync): the
 * editor opens it as a one-layer `SyncStack` that fetches one `bundle` cold and
 * serves list/get from a per-lib IndexedDB cache after that (0 fetches warm,
 * works offline). No resolve endpoint — the layer descriptor is built locally
 * from the lib id + tag, so there's no backend in the loop.
 *
 * Layout under the manifest's directory (`<cdn>/libs/kicad/<tag>/`):
 *   manifest.json            top index: every lib (id, name, kind, itemCount)
 *   <libId>/manifest         per-lib SyncManifest (GET .../manifest)
 *   <libId>/bundle           per-lib bundle: manifest + all bodies (cold init)
 * Item paths inside a lib follow the `"<kind>/<name>"` scheme.
 */

interface CdnLibEntry {
  id: string;
  name: string;
  kind: "symbol" | "footprint";
  itemCount?: number;
  description?: string | null;
}
interface CdnLibsManifest {
  schema: number;
  tag: string;
  libs: CdnLibEntry[];
}

export function cdnLibsSource(
  manifestUrl: string,
  // Test seam: inject fetch + an in-memory store (SyncStack defaults to real
  // fetch + IndexedDB). Production passes nothing.
  opts?: Pick<SyncStackOptions, "fetchImpl" | "storeFactory">,
): LibsSource {
  const baseDir = manifestUrl.replace(/\/[^/]*$/, ""); // <cdn>/libs/kicad/<tag>
  const fetchImpl = opts?.fetchImpl ?? fetch;

  let manifestP: Promise<CdnLibsManifest> | null = null;
  const loadManifest = () =>
    (manifestP ??= (async () => {
      const r = await fetchImpl(manifestUrl, { cache: "no-store" });
      if (!r.ok) throw new Error(`cdn libs manifest ${r.status}: ${manifestUrl}`);
      return (await r.json()) as CdnLibsManifest;
    })());

  // One lazily-opened SyncStack per lib (its IDB store is keyed by namespace, so
  // a lib is cached once and reused across opens).
  const stacks = new Map<string, Promise<SyncStack>>();
  const openStack = (libId: string): Promise<SyncStack> => {
    let p = stacks.get(libId);
    if (!p) {
      p = (async () => {
        const m = await loadManifest();
        const stack = new SyncStack({
          layers: [
            {
              namespace: `kicad:${m.tag}:${libId}`,
              kind: "static",
              url: `${baseDir}/${encodeURIComponent(libId)}`,
            },
          ],
          ...opts,
        });
        await stack.open();
        return stack;
      })();
      stacks.set(libId, p);
    }
    return p;
  };

  return {
    async listLibs(kind?: string): Promise<LibInfo[]> {
      const m = await loadManifest();
      return m.libs
        .filter((l) => !kind || l.kind === kind)
        .map((l) => ({
          id: l.id,
          name: l.name,
          description: l.description ?? null,
          type: "origin",
        }));
    },
    async listItems(libId: string): Promise<LibItemInfo[]> {
      const stack = await openStack(libId);
      return (await stack.list()).map((e) => splitPath(e.path));
    },
    async getItemBody(
      libId: string,
      kind: string,
      name: string,
    ): Promise<string | null> {
      const stack = await openStack(libId);
      const bytes = await stack.read(`${kind}/${name}`);
      return bytes ? new TextDecoder().decode(bytes) : null;
    },
    // Read-only: no saveItemBody / createLib (the demo's default libs are fixed).
  };
}

/** Decode a `"<kind>/<name>"` namespace path into editor item terms. */
function splitPath(path: string): LibItemInfo {
  const i = path.indexOf("/");
  return i < 0
    ? { kind: path, name: "" }
    : { kind: path.slice(0, i), name: path.slice(i + 1) };
}
