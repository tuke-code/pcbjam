/**
 * Read-only session resolution (read-only-viewer).
 *
 * A read-only session runs the editor as a pure viewer: chrome force-hidden,
 * no presence/comments/drift, no save upload, the collab binding never seeds
 * or pushes local edits, and the wasm frame is locked via kicadSetReadOnly
 * (zoom/pan only). The signal is server-authoritative — the project GET's
 * `access` capability field ("read" for callers without write access) — with
 * a `?readonly=1` URL override for tests and authz-free GPL deployments
 * (house `?mobile=` pattern). There is deliberately no `?readonly=0`: a URL
 * parameter must never widen a server-granted capability, and the real
 * enforcement lives in the sync server + wasm gates anyway.
 */

/** The window surface resolveReadOnly reads — narrow, so tests can fake it. */
export interface ReadOnlyWindow {
  location: { search: string };
}

export function resolveReadOnly(
  access: "read" | "write" | undefined,
  win: ReadOnlyWindow = window,
): boolean {
  const param = new URLSearchParams(win.location.search).get("readonly");
  if (param === "1" || param === "true") return true;
  // Absent ⇒ write: authz-free backends never emit the field.
  return access === "read";
}
