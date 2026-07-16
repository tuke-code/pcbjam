import { LOCAL_SCOPE } from "@pcbjam/shared";

/**
 * Non-editor redirect (standalone-hardening 0006): when this deployment has a
 * companion management app (VITE_APP_URL, e.g. https://app.pcbjam.com), every
 * surface the editor host doesn't own — home, the project overview, and any
 * mgmt-only path (/login, /libs, /:scope, …) — bounces there. The two hosts
 * share the path grammar (docs/features/scopes), so the redirect preserves
 * path + search verbatim. Only the actual editor surfaces stay local:
 *
 *   /:scope/projects/:name/-/:tool      fileless tool boot
 *   /:scope/projects/:name/<file…>      file deep-link
 *   /:scope/libs/:name                  lib editor
 *
 * plus anything under the browser-local pseudo-scope (`@local`) — IDB virtual
 * projects have no mgmt counterpart.
 *
 * Known grammar ambiguity: mgmt sub-project pages (…/board/libs, …/board/drift)
 * are indistinguishable from file deep-links, so they stay local — the mgmt app
 * links to those on its own host, never through the editor.
 *
 * Returns the absolute URL to redirect to, or null to render locally.
 * `appUrl` unset (dev / demo builds) ⇒ always null, today's behavior.
 */
export function redirectTargetFor(
  appUrl: string | null,
  pathname: string,
  search = "",
): string | null {
  if (!appUrl) return null;
  const segs = pathname.split("/").filter(Boolean);
  const scope = segs[0] ? decodeURIComponent(segs[0]) : null;
  if (scope === LOCAL_SCOPE) return null;
  if (segs[1] === "projects" && segs.length >= 4) return null;
  if (segs[1] === "libs" && segs.length === 3) return null;
  return appUrl + pathname + search;
}
