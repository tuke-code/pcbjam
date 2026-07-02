import { describe, expect, it } from "vitest";
import { sha256Hex, type SyncManifest } from "@pcbjam/shared";
import { memStore, type LayerStore } from "@pcbjam/sync-client";
import { cdnModelsSource } from "./models-source";

const MANIFEST_URL = "https://cdn.test/libs/kicad-models/9.0.9/manifest.json";
const BASE = "https://cdn.test/libs/kicad-models/9.0.9";
const BLOBS = "https://cdn.test/libs/kicad-models/blobs/sha256";

const enc = new TextEncoder();
const dec = new TextDecoder();

/** Build the CDN layout the way publish-models.ts does: per-lib sparse manifest
 *  + content-addressed blobs — pinning the format with the REAL hash codec. */
async function fakeModelsCdn() {
  const bodies: Record<string, Uint8Array> = {
    "model3d/R_Axial.step": enc.encode("STEP R_Axial"),
    "model3d/R_Disc.wrl": enc.encode("#VRML V2.0 utf8 R_Disc"),
  };
  const entries: SyncManifest["entries"] = {};
  const blobs = new Map<string, Uint8Array>();
  for (const [path, body] of Object.entries(bodies)) {
    const hash = await sha256Hex(body);
    entries[path] = { hash, size: body.length, mtime: 0 };
    blobs.set(hash, body);
  }
  const libManifest: SyncManifest = { version: 1, entries };
  const top = {
    schema: 1,
    tag: "9.0.9",
    libs: [{ id: "Resistor_THT", itemCount: 2 }],
  };

  let manifestFetches = 0;
  let blobFetches = 0;
  const json = (obj: unknown) => ({ ok: true, json: async () => obj });
  const bin = (bytes: Uint8Array) => ({
    ok: true,
    arrayBuffer: async () => bytes.buffer,
  });
  const fetchImpl = (async (url: string) => {
    if (url === MANIFEST_URL) return json(top);
    if (url === `${BASE}/Resistor_THT/manifest`) {
      manifestFetches += 1;
      return json(libManifest);
    }
    if (url.startsWith(`${BLOBS}/`)) {
      const blob = blobs.get(url.slice(`${BLOBS}/`.length));
      if (blob) {
        blobFetches += 1;
        return bin(blob);
      }
    }
    return { ok: false, status: 404 };
  }) as unknown as typeof fetch;

  return {
    fetchImpl,
    counters: {
      get manifestFetches() {
        return manifestFetches;
      },
      get blobFetches() {
        return blobFetches;
      },
    },
  };
}

function storeMap() {
  const stores = new Map<string, LayerStore>();
  return (ns: string): LayerStore => {
    let s = stores.get(ns);
    if (!s) stores.set(ns, (s = memStore()));
    return s;
  };
}

describe("cdnModelsSource", () => {
  it("fetches exactly the requested body (sparse), then serves from cache", async () => {
    const cdn = await fakeModelsCdn();
    const src = cdnModelsSource(MANIFEST_URL, {
      fetchImpl: cdn.fetchImpl,
      storeFactory: storeMap(),
    });

    const body = await src.getModelBody("Resistor_THT.3dshapes/R_Axial.step");
    expect(dec.decode(body!)).toBe("STEP R_Axial");
    expect(cdn.counters.blobFetches).toBe(1); // only the asked-for model

    await src.getModelBody("Resistor_THT.3dshapes/R_Axial.step");
    expect(cdn.counters.blobFetches).toBe(1); // cached
    expect(cdn.counters.manifestFetches).toBe(1); // lib opened once
  });

  it("returns null for unknown models/libs/refs without throwing", async () => {
    const cdn = await fakeModelsCdn();
    const src = cdnModelsSource(MANIFEST_URL, {
      fetchImpl: cdn.fetchImpl,
      storeFactory: storeMap(),
    });

    expect(await src.getModelBody("Resistor_THT.3dshapes/nope.step")).toBeNull();
    expect(await src.getModelBody("NoSuchLib.3dshapes/m.wrl")).toBeNull();
    expect(await src.getModelBody("not-a-model-ref")).toBeNull();
    expect(cdn.counters.blobFetches).toBe(0);
  });

  it("hasModel answers from the manifest without fetching bodies", async () => {
    const cdn = await fakeModelsCdn();
    const src = cdnModelsSource(MANIFEST_URL, {
      fetchImpl: cdn.fetchImpl,
      storeFactory: storeMap(),
    });

    expect(await src.hasModel("Resistor_THT.3dshapes/R_Disc.wrl")).toBe(true);
    expect(await src.hasModel("Resistor_THT.3dshapes/nope.wrl")).toBe(false);
    expect(cdn.counters.blobFetches).toBe(0);
  });
});
