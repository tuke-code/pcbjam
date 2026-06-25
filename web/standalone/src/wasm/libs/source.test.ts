import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { installLibsProvider, type LibsSource } from "./source";
import { libUri } from "./uri";

/**
 * The WASM lib plugins call `window.kicadLibs.request(op, lib, arg, kind)`. These
 * tests pin the wire contract the C++ side parses — in particular the "fat list"
 * (`arg === "bodies"`) shape `{symbols|footprints: [{name, body}]}` and its
 * fallback for sources without a bulk `getAllItems`. Node env has no `window`, so
 * we stub the minimal surface the provider touches.
 */

type Req = (
  op: string,
  lib: string,
  arg: string,
  kind?: string,
) => Promise<string | null>;

function installAndGetRequest(source: LibsSource): Req {
  installLibsProvider(source, () => {});
  return (globalThis as unknown as { window: { kicadLibs: { request: Req } } })
    .window.kicadLibs.request;
}

const ITEMS = [
  { kind: "symbol", name: "R", body: "(kicad_symbol_lib (symbol R))" },
  { kind: "symbol", name: "C", body: "(kicad_symbol_lib (symbol C))" },
  { kind: "footprint", name: "R_0402", body: "(footprint R_0402)" },
];

function baseSource(over: Partial<LibsSource> = {}): LibsSource {
  return {
    listLibs: async () => [],
    listItems: async () => ITEMS.map((i) => ({ kind: i.kind, name: i.name })),
    getItemBody: async (_id, kind, name) =>
      ITEMS.find((i) => i.kind === kind && i.name === name)?.body ?? null,
    ...over,
  };
}

describe("installLibsProvider — fat list (arg=bodies)", () => {
  beforeEach(() => {
    (globalThis as unknown as { window: unknown }).window = {
      location: { search: "" },
      dispatchEvent: () => true,
    };
  });
  afterEach(() => {
    delete (globalThis as unknown as { window?: unknown }).window;
  });

  it("returns names+bodies for the requested kind via getAllItems", async () => {
    const calls: string[] = [];
    const request = installAndGetRequest(
      baseSource({
        getAllItems: async (id) => {
          calls.push(id);
          return ITEMS;
        },
      }),
    );

    const res = await request("list", libUri("Device"), "bodies", "symbol");
    expect(JSON.parse(res!)).toEqual({
      symbols: [
        { name: "R", body: "(kicad_symbol_lib (symbol R))" },
        { name: "C", body: "(kicad_symbol_lib (symbol C))" },
      ],
    });
    // One bulk call, not per-item.
    expect(calls).toEqual(["Device"]);

    const fps = await request("list", libUri("Device"), "bodies", "footprint");
    expect(JSON.parse(fps!)).toEqual({
      footprints: [{ name: "R_0402", body: "(footprint R_0402)" }],
    });
  });

  it("falls back to listItems + getItemBody when getAllItems is absent", async () => {
    const request = installAndGetRequest(baseSource()); // no getAllItems
    const res = await request("list", libUri("Device"), "bodies", "symbol");
    expect(JSON.parse(res!)).toEqual({
      symbols: [
        { name: "R", body: "(kicad_symbol_lib (symbol R))" },
        { name: "C", body: "(kicad_symbol_lib (symbol C))" },
      ],
    });
  });

  it("plain list (empty arg) still returns names only", async () => {
    const request = installAndGetRequest(
      baseSource({ getAllItems: async () => ITEMS }),
    );
    const res = await request("list", libUri("Device"), "", "symbol");
    expect(JSON.parse(res!)).toEqual({ symbols: ["R", "C"] });
  });
});
