import { PLAUSIBLE_DOMAIN, PLAUSIBLE_SRC } from "./config";

/**
 * Inject the Plausible analytics script when a domain is configured
 * (VITE_PLAUSIBLE_DOMAIN). No-op otherwise, so a plain dev checkout — and any
 * deploy that doesn't set the var — stays untracked. Cookieless + privacy-
 * friendly, so no consent banner is required.
 *
 * `crossorigin="anonymous"` is required because the demo is served cross-origin
 * isolated (COEP `require-corp`): a third-party script must be fetched with CORS.
 * If plausible.io ever fails to load under that policy, point VITE_PLAUSIBLE_SRC
 * at a self-hosted/proxied copy on cdn.pcbjam.com (which sets CORP).
 */
export function initAnalytics(): void {
  if (!PLAUSIBLE_DOMAIN || typeof document === "undefined") return;
  if (document.querySelector("script[data-pcbjam-analytics]")) return;
  const s = document.createElement("script");
  s.defer = true;
  s.setAttribute("data-domain", PLAUSIBLE_DOMAIN);
  s.setAttribute("data-pcbjam-analytics", "");
  s.crossOrigin = "anonymous";
  s.src = PLAUSIBLE_SRC;
  document.head.appendChild(s);
}
