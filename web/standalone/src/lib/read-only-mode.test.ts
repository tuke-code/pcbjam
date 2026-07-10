import { describe, expect, it } from "vitest";
import { resolveReadOnly, type ReadOnlyWindow } from "./read-only-mode";

/**
 * Read-only session resolution (read-only-viewer): the server-granted
 * `access` capability decides; `?readonly=1` can only NARROW (force viewer
 * mode for tests / authz-free deployments), never widen.
 */

function fakeWin(search = ""): ReadOnlyWindow {
  return { location: { search } };
}

describe("resolveReadOnly", () => {
  it("follows the server capability", () => {
    expect(resolveReadOnly("read", fakeWin())).toBe(true);
    expect(resolveReadOnly("write", fakeWin())).toBe(false);
  });

  it("treats an absent capability as writable (authz-free backends)", () => {
    expect(resolveReadOnly(undefined, fakeWin())).toBe(false);
  });

  it("?readonly=1 forces viewer mode regardless of capability", () => {
    expect(resolveReadOnly(undefined, fakeWin("?readonly=1"))).toBe(true);
    expect(resolveReadOnly("write", fakeWin("?foo=bar&readonly=true"))).toBe(true);
  });

  it("has no ?readonly=0 escape — a URL never widens a server grant", () => {
    expect(resolveReadOnly("read", fakeWin("?readonly=0"))).toBe(true);
    expect(resolveReadOnly("read", fakeWin("?readonly=false"))).toBe(true);
  });
});
