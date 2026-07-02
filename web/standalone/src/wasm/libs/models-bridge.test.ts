import { describe, expect, it } from "vitest";
import { normalizeModelRef, scanModelRefs } from "./models-bridge";

describe("normalizeModelRef", () => {
  it("strips any vintage of the model-dir var", () => {
    for (const v of ["KICAD6", "KICAD7", "KICAD8", "KICAD9", "KICAD10"]) {
      expect(
        normalizeModelRef(`\${${v}_3DMODEL_DIR}/Resistor_SMD.3dshapes/R_0201.wrl`),
      ).toBe("Resistor_SMD.3dshapes/R_0201.wrl");
    }
  });

  it("accepts the paren syntax and the legacy KISYS3DMOD alias", () => {
    expect(normalizeModelRef("$(KICAD8_3DMODEL_DIR)/L.3dshapes/m.step")).toBe(
      "L.3dshapes/m.step",
    );
    expect(normalizeModelRef("${KISYS3DMOD}/L.3dshapes/m.wrl")).toBe(
      "L.3dshapes/m.wrl",
    );
  });

  it("passes bare relative refs through", () => {
    expect(normalizeModelRef("L.3dshapes/m.wrl")).toBe("L.3dshapes/m.wrl");
  });

  it("rejects refs it cannot serve", () => {
    expect(normalizeModelRef("${KIPRJMOD}/libs/3d/m.wrl")).toBeNull(); // project-local
    expect(normalizeModelRef("/abs/path/m.wrl")).toBeNull();
    expect(normalizeModelRef("kicad_embed://m.wrl")).toBeNull();
    expect(normalizeModelRef("")).toBeNull();
    expect(normalizeModelRef("${UNCLOSED/m.wrl")).toBeNull();
  });
});

describe("scanModelRefs", () => {
  it("finds, normalizes and dedupes board model refs", () => {
    const board = `
      (footprint "Resistor_THT:R_Axial"
        (model "\${KICAD10_3DMODEL_DIR}/Resistor_THT.3dshapes/R_Axial.step"
          (offset (xyz 0 0 0))))
      (footprint "Resistor_THT:R_Axial"
        (model "\${KICAD10_3DMODEL_DIR}/Resistor_THT.3dshapes/R_Axial.step"))
      (footprint "X:Y"
        (model "\${KIPRJMOD}/libs/3d_shapes/custom.wrl"))
      (footprint "L:M" (model "\${KICAD8_3DMODEL_DIR}/LED_THT.3dshapes/LED_D5.0mm.wrl"))
    `;
    expect(scanModelRefs(board).sort()).toEqual([
      "LED_THT.3dshapes/LED_D5.0mm.wrl",
      "Resistor_THT.3dshapes/R_Axial.step",
    ]);
  });

  it("handles escaped quotes inside the path", () => {
    expect(
      scanModelRefs('(model "${KICAD9_3DMODEL_DIR}/A.3dshapes/we\\"ird.wrl")'),
    ).toEqual(['A.3dshapes/we"ird.wrl']);
  });

  it("returns empty for a board with no models", () => {
    expect(scanModelRefs("(kicad_pcb (version 20240101))")).toEqual([]);
  });
});
