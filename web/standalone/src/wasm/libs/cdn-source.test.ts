import { describe, expect, it } from "vitest";
import { encodeBundle, sha256Hex, type SyncManifest } from "@pcbjam/shared";
import { memStore } from "@pcbjam/sync-client";
import { cdnLibsSource } from "./cdn-source";

const MANIFEST_URL = "https://cdn.test/libs/kicad/9.0.0/manifest.json";
const BASE = "https://cdn.test/libs/kicad/9.0.0";

const enc = new TextEncoder();

/** Build a static-origin snapshot (per-lib manifest + bundle) the way
 *  publish-libs will, using the REAL wire codecs — so this pins the format. */
async function makeLib(items: Record<string, string>) {
  const bodies = Object.entries(items).map(
    ([path, text]): [string, Uint8Array] => [path, enc.encode(text)],
  );
  const entries: SyncManifest["entries"] = {};
  for (const [path, body] of bodies) {
    entries[path] = { hash: await sha256Hex(body), size: body.length, mtime: 0 };
  }
  const manifest: SyncManifest = { version: 1, entries };
  return { manifest, bundle: encodeBundle(manifest, bodies) };
}

async function fakeCdn() {
  const device = await makeLib({
    "symbol/R": "(kicad_symbol_lib (symbol R))",
    "symbol/C": "(kicad_symbol_lib (symbol C))",
  });
  const resistors = await makeLib({
    "footprint/R_0402_1005Metric": "(footprint R_0402)",
  });
  const top = {
    schema: 1,
    tag: "9.0.0",
    libs: [
      { id: "Device", name: "Device", kind: "symbol", itemCount: 2 },
      { id: "Resistor_SMD", name: "Resistor_SMD", kind: "footprint", itemCount: 1 },
    ],
  };
  const json = (obj: unknown) => ({ ok: true, json: async () => obj });
  const bin = (bytes: Uint8Array) => ({
    ok: true,
    arrayBuffer: async () => bytes.buffer,
  });
  const fetchImpl = (async (url: string) => {
    if (url === MANIFEST_URL) return json(top);
    if (url === `${BASE}/Device/manifest`) return json(device.manifest);
    if (url === `${BASE}/Device/bundle`) return bin(device.bundle);
    if (url === `${BASE}/Resistor_SMD/manifest`) return json(resistors.manifest);
    if (url === `${BASE}/Resistor_SMD/bundle`) return bin(resistors.bundle);
    return { ok: false, status: 404 };
  }) as unknown as typeof fetch;
  return cdnLibsSource(MANIFEST_URL, {
    fetchImpl,
    storeFactory: () => memStore(),
  });
}

describe("cdn libs source", () => {
  it("lists libs from the top manifest, filtered by kind", async () => {
    const src = await fakeCdn();
    expect((await src.listLibs()).map((l) => l.id)).toEqual([
      "Device",
      "Resistor_SMD",
    ]);
    expect((await src.listLibs("symbol")).map((l) => l.id)).toEqual(["Device"]);
    expect((await src.listLibs("footprint")).map((l) => l.id)).toEqual([
      "Resistor_SMD",
    ]);
  });

  it("lists a lib's items from one cold bundle fetch", async () => {
    const src = await fakeCdn();
    const items = await src.listItems("Device");
    expect(items.sort((a, b) => a.name.localeCompare(b.name))).toEqual([
      { kind: "symbol", name: "C" },
      { kind: "symbol", name: "R" },
    ]);
  });

  it("returns a self-contained item body by kind/name", async () => {
    const src = await fakeCdn();
    expect(await src.getItemBody("Device", "symbol", "R")).toBe(
      "(kicad_symbol_lib (symbol R))",
    );
    expect(
      await src.getItemBody("Resistor_SMD", "footprint", "R_0402_1005Metric"),
    ).toBe("(footprint R_0402)");
    expect(await src.getItemBody("Device", "symbol", "Nope")).toBeNull();
  });

  it("is read-only (no save path)", async () => {
    const src = await fakeCdn();
    expect(src.saveItemBody).toBeUndefined();
  });
});
