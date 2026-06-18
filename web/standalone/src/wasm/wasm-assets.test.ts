import { afterEach, describe, expect, it, vi } from "vitest";
import type { Tool } from "@pcbjam/shared";

// resolveWasmBase reads WASM_ROOT / WASM_MANIFEST_FILE from config at import
// time, so each case mocks config fresh then dynamically imports the module.
async function loadResolver(cfg: {
  WASM_ROOT: string;
  WASM_MANIFEST_FILE: string | null;
}) {
  vi.resetModules();
  vi.doMock("@/lib/config", () => cfg);
  return (await import("./wasm-assets")).resolveWasmBase;
}

const PCBNEW = "pcbnew" as Tool;

afterEach(() => {
  vi.unstubAllGlobals();
  vi.restoreAllMocks();
});

describe("resolveWasmBase", () => {
  it("uses an explicit override verbatim (trailing slash stripped), no fetch", async () => {
    const fetchMock = vi.fn();
    vi.stubGlobal("fetch", fetchMock);
    const resolve = await loadResolver({
      WASM_ROOT: "/wasm",
      WASM_MANIFEST_FILE: "manifest-1.json",
    });
    expect(await resolve(PCBNEW, "https://cdn.example/wasm/pcbnew/9/")).toBe(
      "https://cdn.example/wasm/pcbnew/9",
    );
    expect(fetchMock).not.toHaveBeenCalled();
  });

  it("returns the flat root when no manifest is configured (dev), no fetch", async () => {
    const fetchMock = vi.fn();
    vi.stubGlobal("fetch", fetchMock);
    const resolve = await loadResolver({
      WASM_ROOT: "/wasm",
      WASM_MANIFEST_FILE: null,
    });
    expect(await resolve(PCBNEW)).toBe("/wasm");
    expect(fetchMock).not.toHaveBeenCalled();
  });

  it("resolves the per-tool versioned folder from the manifest", async () => {
    const fetchMock = vi.fn(async () => ({
      ok: true,
      json: async () => ({
        schema: 1,
        tag: "2.7.7",
        tools: { pcbnew: "2.7.5", eeschema: "2.7.1" },
      }),
    }));
    vi.stubGlobal("fetch", fetchMock);
    const resolve = await loadResolver({
      WASM_ROOT: "https://cdn.pcbjam.com/wasm",
      WASM_MANIFEST_FILE: "manifest-2.7.7.json",
    });
    expect(await resolve(PCBNEW)).toBe("https://cdn.pcbjam.com/wasm/pcbnew/2.7.5");
    // Manifest is fetched uncached, and only ONCE across calls (in-memory cached).
    expect(await resolve("eeschema" as Tool)).toBe(
      "https://cdn.pcbjam.com/wasm/eeschema/2.7.1",
    );
    expect(fetchMock).toHaveBeenCalledTimes(1);
    expect(fetchMock).toHaveBeenCalledWith(
      "https://cdn.pcbjam.com/wasm/manifest-2.7.7.json",
      { cache: "no-store" },
    );
  });

  it("throws when the manifest omits the tool", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn(async () => ({ ok: true, json: async () => ({ tools: {} }) })),
    );
    const resolve = await loadResolver({
      WASM_ROOT: "https://cdn.pcbjam.com/wasm",
      WASM_MANIFEST_FILE: "manifest-2.7.7.json",
    });
    await expect(resolve(PCBNEW)).rejects.toThrow(/no WASM version for "pcbnew"/);
  });

  it("throws when the manifest fetch fails", async () => {
    vi.stubGlobal("fetch", vi.fn(async () => ({ ok: false, status: 404 })));
    const resolve = await loadResolver({
      WASM_ROOT: "https://cdn.pcbjam.com/wasm",
      WASM_MANIFEST_FILE: "manifest-2.7.7.json",
    });
    await expect(resolve(PCBNEW)).rejects.toThrow(/WASM manifest 404/);
  });
});
