import type { LibInfo, LibItemInfo, LibsSource } from "./source";

/**
 * Wrap a `LibsSource` so the editor sees exactly ONE library — the lib-table
 * gets a single row, instead of "browse all backend libs". Used to open a
 * specific backend library scoped to itself (HomePage → a lib chip).
 *
 * Read-only scope: `listItems`/`getItemBody` forward to the base for the target
 * lib; no `createLib`/`saveItemBody` is exposed (the editor opens this origin to
 * browse, not to write — matching today's read-only-origin behavior).
 */
export function scopedLibsSource(base: LibsSource, libId: string): LibsSource {
  return {
    async listLibs(kind?: string): Promise<LibInfo[]> {
      const all = await base.listLibs(kind);
      return all.filter((l) => l.id === libId);
    },
    listItems(id: string): Promise<LibItemInfo[]> {
      return base.listItems(id);
    },
    getItemBody(id: string, kind: string, name: string): Promise<string | null> {
      return base.getItemBody(id, kind, name);
    },
    // Forward the bulk "fat list" only when the base supports it, so a scoped
    // single-lib view keeps the one-crossing hydrate (else the provider's slow
    // fallback applies).
    ...(base.getAllItems
      ? { getAllItems: (id: string) => base.getAllItems!(id) }
      : {}),
    // Pre-sync ONLY the scoped lib — NOT the base's whole catalog (that's what a
    // bare `base.presync()` would do). Opening the one lib via listItems warms its
    // bundle into IDB. Gated on the base being cache-capable (it exposes presync);
    // per-item remote has no client cache to warm.
    ...(base.presync
      ? {
          presync: async (
            o?: Parameters<NonNullable<LibsSource["presync"]>>[0],
          ): Promise<void> => {
            o?.onProgress?.({ done: 0, total: 1, current: libId });
            try {
              await base.listItems(libId);
            } catch {
              // best-effort: the lib still loads lazily on demand
            }
            o?.onProgress?.({ done: 1, total: 1, current: libId });
          },
        }
      : {}),
  };
}
