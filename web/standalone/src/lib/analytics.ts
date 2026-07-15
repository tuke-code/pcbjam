import { PLAUSIBLE_SRC } from "./config";

type PlausibleShim = {
  (...args: unknown[]): void;
  q?: unknown[][];
  init?: (options?: object) => void;
  o?: object;
};

/**
 * Inject the Plausible analytics script when a script URL is configured
 * (VITE_PLAUSIBLE_SRC, the site's pa-*.js URL — the pa- id keys the shared
 * dashboard, so no data-domain). No-op otherwise, so a plain dev checkout —
 * and any deploy that doesn't set the var — stays untracked. Cookieless +
 * privacy-friendly, so no consent banner is required.
 *
 * `crossorigin="anonymous"` is required because the demo is served cross-origin
 * isolated (COEP `require-corp`): a third-party script must be fetched with CORS.
 * If plausible.io ever fails to load under that policy, point VITE_PLAUSIBLE_SRC
 * at a self-hosted/proxied copy on cdn.pcbjam.com (which sets CORP).
 */
export function initAnalytics(): void {
  if (!PLAUSIBLE_SRC || typeof document === "undefined") return;
  if (document.querySelector("script[data-pcbjam-analytics]")) return;
  // Official queue shim: buffers plausible() calls until the script loads.
  const w = window as Window & { plausible?: PlausibleShim };
  const p: PlausibleShim =
    w.plausible ||
    ((...args: unknown[]) => {
      (p.q = p.q || []).push(args);
    });
  p.init =
    p.init ||
    ((options?: object) => {
      p.o = options || {};
    });
  w.plausible = p;
  p.init();
  const s = document.createElement("script");
  s.async = true;
  s.setAttribute("data-pcbjam-analytics", "");
  s.crossOrigin = "anonymous";
  s.src = PLAUSIBLE_SRC;
  document.head.appendChild(s);
}
