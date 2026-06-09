export const API_BASE_URL =
  import.meta.env.VITE_API_BASE_URL ?? "http://localhost:3050";

// Default is SAME-ORIGIN ("/wasm", served from public/wasm by Vite). KiCad WASM
// pthread workers cannot be created cross-origin, so dev must serve same-origin.
// Override with an absolute URL (e.g. a CDN) only if that origin is configured
// to also satisfy the worker/COEP constraints.
export const WASM_ASSET_BASE_URL =
  import.meta.env.VITE_WASM_ASSET_BASE_URL ?? "/wasm";
