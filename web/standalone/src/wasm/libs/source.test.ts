import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { installLibsProvider, type LibsSource } from "./source";
import { libUri } from "./uri";

/**
 * The WASM lib plugins call `window.kicadLibs.request(op, lib, arg, kind)`. These
 * tests pin the wire contract the C++ side parses — in particular the "fat list"
 * (`arg === "bodies"`), which is the "copy as-is" FRAMED payload: a one-line JSON
 * header `{symbols|footprints: [{name, len}]}`, a newline, then every body's raw
 * bytes concatenated (no JSON escaping). The bodies cross as a `Uint8Array` the
 * bridge memcpy's straight into the wasm heap. We also cover the fallback for
 * sources without a bulk `getAllItems`. Node env has no `window`, so we stub the
 * minimal surface the provider touches.
 */

type Req = (
  op: string,
  lib: string,
  arg: string,
  kind?: string,
) => Promise<string | Uint8Array | null>;

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

const enc = new TextEncoder();
const dec = new TextDecoder();

/** getAllItems bodies now cross as raw bytes (copied as-is, no TextDecoder). */
function itemsAsBytes(items = ITEMS) {
  return items.map((i) => ({
    kind: i.kind,
    name: i.name,
    body: enc.encode(i.body),
  }));
}

/**
 * Decode the framed fat-list payload back into `{name, body}` records so the
 * assertions read the same as the underlying data. Mirrors what the C++ `fatLoad`
 * does: split at the first `\n`, parse the header, slice bodies by byte length.
 */
function parseFramed(
  res: string | Uint8Array | null,
  key: "symbols" | "footprints",
): Array<{ name: string; body: string }> {
  expect(res).toBeInstanceOf(Uint8Array);
  const bytes = res as Uint8Array;
  const nl = bytes.indexOf(0x0a);
  expect(nl).toBeGreaterThan(0);
  const header = JSON.parse(dec.decode(bytes.subarray(0, nl))) as {
    [k: string]: Array<{ name: string; len: number }>;
  };
  let off = nl + 1;
  return (header[key] ?? []).map(({ name, len }) => {
    const body = dec.decode(bytes.subarray(off, off + len));
    off += len;
    return { name, body };
  });
}

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

  it("returns a framed names+bodies payload for the requested kind via getAllItems", async () => {
    const calls: string[] = [];
    const request = installAndGetRequest(
      baseSource({
        getAllItems: async (id) => {
          calls.push(id);
          return itemsAsBytes();
        },
      }),
    );

    const res = await request("list", libUri("Device"), "bodies", "symbol");
    expect(parseFramed(res, "symbols")).toEqual([
      { name: "R", body: "(kicad_symbol_lib (symbol R))" },
      { name: "C", body: "(kicad_symbol_lib (symbol C))" },
    ]);
    // One bulk call, not per-item.
    expect(calls).toEqual(["Device"]);

    const fps = await request("list", libUri("Device"), "bodies", "footprint");
    expect(parseFramed(fps, "footprints")).toEqual([
      { name: "R_0402", body: "(footprint R_0402)" },
    ]);
  });

  it("falls back to listItems + getItemBody when getAllItems is absent", async () => {
    const request = installAndGetRequest(baseSource()); // no getAllItems
    const res = await request("list", libUri("Device"), "bodies", "symbol");
    expect(parseFramed(res, "symbols")).toEqual([
      { name: "R", body: "(kicad_symbol_lib (symbol R))" },
      { name: "C", body: "(kicad_symbol_lib (symbol C))" },
    ]);
  });

  it("plain list (empty arg) still returns names only", async () => {
    const request = installAndGetRequest(
      baseSource({ getAllItems: async () => itemsAsBytes() }),
    );
    const res = await request("list", libUri("Device"), "", "symbol");
    expect(JSON.parse(res as string)).toEqual({ symbols: ["R", "C"] });
  });
});
