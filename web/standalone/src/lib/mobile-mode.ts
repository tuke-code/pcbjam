/**
 * Mobile-mode resolution (features/mobile).
 *
 * In mobile mode the editor runs canvas-only: the web shell hides its overlays
 * (version badge, source chip, console toggle), boot installs the touch-gesture
 * shim, and the wasm hides the editor chrome (kicadSetChrome). One shared
 * signal so every consumer agrees:
 *
 *   - `?mobile=1` / `?mobile=0` (also true/false) override everything — the
 *     deterministic switch for tests and for users on unusual devices.
 *   - otherwise auto-detect: UA-CH `userAgentData.mobile`, or coarse pointer +
 *     narrow viewport (the same signals capabilities.ts warns on).
 */

/** The window surface isMobileMode reads — narrow, so tests can fake it. */
export interface MobileModeWindow {
  location: { search: string };
  navigator?: { userAgentData?: { mobile?: boolean } };
  matchMedia?: ((query: string) => { matches: boolean }) | undefined;
}

export function isMobileMode(
  // same narrow-cast pattern as capabilities.ts: userAgentData is not in lib.dom
  win: MobileModeWindow = window as unknown as MobileModeWindow,
): boolean {
  const param = new URLSearchParams(win.location.search).get("mobile");
  if (param === "0" || param === "false") return false;
  if (param === "1" || param === "true") return true;

  if (win.navigator?.userAgentData?.mobile === true) return true;

  const mm = win.matchMedia;
  if (typeof mm !== "function") return false;
  return mm("(pointer: coarse)").matches && mm("(max-width: 900px)").matches;
}
