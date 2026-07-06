import { describe, expect, it } from "vitest";
import { isMobileMode, type MobileModeWindow } from "./mobile-mode";

/**
 * Mobile-mode resolution (features/mobile): the explicit `?mobile=` URL param
 * always wins; otherwise fall back to device detection (UA-CH mobile flag, or
 * coarse pointer + narrow viewport — the same signals capabilities.ts warns on).
 */

function fakeWin(opts: {
  search?: string;
  uaMobile?: boolean;
  coarse?: boolean;
  narrow?: boolean;
  noMatchMedia?: boolean;
}): MobileModeWindow {
  return {
    location: { search: opts.search ?? "" },
    navigator: { userAgentData: opts.uaMobile === undefined ? undefined : { mobile: opts.uaMobile } },
    matchMedia: opts.noMatchMedia
      ? undefined
      : (query: string) => ({
          matches: query.includes("pointer") ? (opts.coarse ?? false) : (opts.narrow ?? false),
        }),
  };
}

describe("isMobileMode", () => {
  it("?mobile=1 forces mobile mode on a desktop device", () => {
    expect(isMobileMode(fakeWin({ search: "?mobile=1" }))).toBe(true);
    expect(isMobileMode(fakeWin({ search: "?foo=bar&mobile=true" }))).toBe(true);
  });

  it("?mobile=0 forces desktop mode on a mobile device", () => {
    expect(
      isMobileMode(fakeWin({ search: "?mobile=0", uaMobile: true, coarse: true, narrow: true })),
    ).toBe(false);
    expect(isMobileMode(fakeWin({ search: "?mobile=false", uaMobile: true }))).toBe(false);
  });

  it("auto-detects via userAgentData.mobile", () => {
    expect(isMobileMode(fakeWin({ uaMobile: true }))).toBe(true);
    expect(isMobileMode(fakeWin({ uaMobile: false }))).toBe(false);
  });

  it("auto-detects via coarse pointer + narrow viewport", () => {
    expect(isMobileMode(fakeWin({ coarse: true, narrow: true }))).toBe(true);
    // a touch-screen desktop (coarse but wide) is NOT mobile
    expect(isMobileMode(fakeWin({ coarse: true, narrow: false }))).toBe(false);
    expect(isMobileMode(fakeWin({ coarse: false, narrow: true }))).toBe(false);
  });

  it("defaults to desktop when nothing is detectable", () => {
    expect(isMobileMode(fakeWin({}))).toBe(false);
    expect(isMobileMode(fakeWin({ noMatchMedia: true }))).toBe(false);
  });
});
