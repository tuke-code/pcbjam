import { describe, expect, it } from "vitest";
import {
  TouchGestureRecognizer,
  type GestureAction,
  type TouchPt,
} from "./touch-gestures";

/**
 * TDD spec for the mobile touch-gesture recognizer (features/mobile).
 *
 * The recognizer is a pure state machine: it receives the ACTIVE touch list
 * (the shape of `TouchEvent.touches`) plus a timestamp on every touch event,
 * and emits abstract actions the boot shim translates into the editor's
 * proven input paths:
 *   - pan-*  → synthetic middle-button drag (WX_VIEW_CONTROLS DRAG_PANNING)
 *   - zoom   → synthetic wheel at the pinch centroid (zoom-to-cursor)
 *   - tap    → synthetic left click (selection)
 */

const t = (id: number, x: number, y: number): TouchPt => ({ id, x, y });

/** One recognizer with deterministic defaults for tests. */
function rec() {
  return new TouchGestureRecognizer({
    tapMaxMs: 300,
    tapMaxDist: 10,
    zoomSensitivity: 3,
    minWheelDelta: 15,
  });
}

function kinds(actions: GestureAction[]): string[] {
  return actions.map((a) => a.kind);
}

describe("tap", () => {
  it("quick touch without movement emits a single tap at the touch point", () => {
    const r = rec();
    expect(r.update([t(1, 100, 100)], 0)).toEqual([]);
    expect(r.update([], 150)).toEqual([{ kind: "tap", x: 100, y: 100 }]);
  });

  it("tolerates sub-threshold jitter", () => {
    const r = rec();
    r.update([t(1, 100, 100)], 0);
    expect(r.update([t(1, 104, 103)], 50)).toEqual([]);
    expect(kinds(r.update([], 120))).toEqual(["tap"]);
  });

  it("a long still press emits nothing", () => {
    const r = rec();
    r.update([t(1, 100, 100)], 0);
    expect(r.update([], 500)).toEqual([]);
  });
});

describe("one-finger pan", () => {
  it("starts panning once movement exceeds the tap threshold, anchored at the ORIGINAL touch point", () => {
    const r = rec();
    r.update([t(1, 100, 100)], 0);
    expect(r.update([t(1, 105, 100)], 20)).toEqual([]); // below threshold
    expect(r.update([t(1, 130, 100)], 40)).toEqual([
      { kind: "pan-start", x: 100, y: 100 },
      { kind: "pan-move", x: 130, y: 100 },
    ]);
    expect(r.update([t(1, 150, 120)], 60)).toEqual([
      { kind: "pan-move", x: 150, y: 120 },
    ]);
    expect(r.update([], 80)).toEqual([{ kind: "pan-end", x: 150, y: 120 }]);
  });

  it("a slow drag is still a pan (time does not demote it to a tap)", () => {
    const r = rec();
    r.update([t(1, 0, 0)], 0);
    r.update([t(1, 50, 0)], 1000);
    expect(kinds(r.update([], 2000))).toEqual(["pan-end"]);
  });
});

describe("pinch zoom", () => {
  it("pinch-out emits negative wheel deltas (zoom in) at the centroid, totalling ~sensitivity*120 per doubling", () => {
    const r = rec();
    r.update([t(1, 100, 200), t(2, 200, 200)], 0); // dist 100, centroid (150,200)
    const actions: GestureAction[] = [];
    // widen 100 → 200 in 10 steps
    for (let i = 1; i <= 10; i++) {
      const spread = 100 + i * 10;
      actions.push(
        ...r.update(
          [t(1, 150 - spread / 2, 200), t(2, 150 + spread / 2, 200)],
          i * 16,
        ),
      );
    }
    const zooms = actions.filter((a) => a.kind === "zoom");
    expect(zooms.length).toBeGreaterThan(0);
    for (const z of zooms) {
      expect(z.kind).toBe("zoom");
      if (z.kind === "zoom") {
        expect(z.deltaY).toBeLessThan(0); // pinch-out = zoom IN = negative wheel
        expect(z.cy).toBe(200); // centroid stays on the finger axis
      }
    }
    const total = zooms.reduce((s, z) => s + (z.kind === "zoom" ? z.deltaY : 0), 0);
    // one full doubling = sensitivity(3) * 120 = 360, minus at most the
    // un-emitted sub-threshold remainder
    expect(total).toBeLessThanOrEqual(-360 + 15);
    expect(total).toBeGreaterThanOrEqual(-360 - 1e-6);
  });

  it("pinch-in emits positive wheel deltas (zoom out)", () => {
    const r = rec();
    r.update([t(1, 50, 200), t(2, 250, 200)], 0); // dist 200
    const actions = r.update([t(1, 100, 200), t(2, 200, 200)], 16); // dist 100
    const zooms = actions.filter((a) => a.kind === "zoom");
    expect(zooms.length).toBe(1);
    const z = zooms[0];
    if (z?.kind === "zoom") {
      expect(z.deltaY).toBeCloseTo(360, 5);
      expect(z.cx).toBe(150);
    }
  });

  it("accumulates sub-threshold pinch movement instead of dropping it", () => {
    const r = rec();
    r.update([t(1, 0, 0), t(2, 100, 0)], 0); // dist 100
    // +2% (≈ -10.4 deltaY): below the 15 threshold — nothing emitted
    expect(r.update([t(1, 0, 0), t(2, 102, 0)], 16)).toEqual([]);
    // another +2% (cumulative ≈ -21): now emits the ACCUMULATED delta
    const actions = r.update([t(1, 0, 0), t(2, 104.04, 0)], 32);
    expect(kinds(actions)).toEqual(["zoom"]);
    const z = actions[0];
    if (z?.kind === "zoom") {
      expect(z.deltaY).toBeCloseTo(-360 * Math.log2(1.0404), 3);
    }
  });

  it("ignores extra fingers beyond the first two", () => {
    const r = rec();
    r.update([t(1, 0, 0), t(2, 100, 0), t(3, 500, 500)], 0);
    const actions = r.update([t(1, 0, 0), t(2, 200, 0), t(3, 500, 500)], 16);
    const zooms = actions.filter((a) => a.kind === "zoom");
    expect(zooms.length).toBe(1);
    const z = zooms[0];
    if (z?.kind === "zoom") {
      expect(z.cx).toBe(100); // centroid of fingers 1+2 only
      expect(z.cy).toBe(0);
    }
  });
});

describe("finger-count transitions", () => {
  it("1→2: an active pan ends before the pinch starts", () => {
    const r = rec();
    r.update([t(1, 100, 100)], 0);
    r.update([t(1, 150, 100)], 20); // pan active
    expect(r.update([t(1, 150, 100), t(2, 250, 100)], 40)).toEqual([
      { kind: "pan-end", x: 150, y: 100 },
    ]);
    const actions = r.update([t(1, 100, 100), t(2, 300, 100)], 56); // dist 100→200
    expect(kinds(actions)).toEqual(["zoom"]);
  });

  it("1→2 during a pending tap emits nothing (no phantom pan)", () => {
    const r = rec();
    r.update([t(1, 100, 100)], 0);
    expect(r.update([t(1, 100, 100), t(2, 200, 100)], 20)).toEqual([]);
  });

  it("2→1: pinch hands off to a pan anchored at the remaining finger", () => {
    const r = rec();
    r.update([t(1, 100, 100), t(2, 200, 100)], 0);
    expect(r.update([t(2, 200, 100)], 20)).toEqual([
      { kind: "pan-start", x: 200, y: 100 },
    ]);
    expect(r.update([t(2, 220, 110)], 40)).toEqual([
      { kind: "pan-move", x: 220, y: 110 },
    ]);
    expect(r.update([], 60)).toEqual([{ kind: "pan-end", x: 220, y: 110 }]);
  });

  it("2→1→0 quickly does NOT produce a tap", () => {
    const r = rec();
    r.update([t(1, 100, 100), t(2, 200, 100)], 0);
    r.update([t(2, 200, 100)], 10);
    const actions = r.update([], 30);
    expect(kinds(actions)).toEqual(["pan-end"]);
  });

  it("2→0 (both lifted at once) emits nothing", () => {
    const r = rec();
    r.update([t(1, 100, 100), t(2, 200, 100)], 0);
    expect(r.update([], 20)).toEqual([]);
    // and the recognizer is reusable afterwards
    r.update([t(3, 50, 50)], 100);
    expect(kinds(r.update([], 150))).toEqual(["tap"]);
  });
});

describe("cancel", () => {
  it("cancel during a pan ends it", () => {
    const r = rec();
    r.update([t(1, 100, 100)], 0);
    r.update([t(1, 160, 100)], 20);
    expect(r.cancel()).toEqual([{ kind: "pan-end", x: 160, y: 100 }]);
  });

  it("cancel during a pending tap or pinch emits nothing and resets", () => {
    const r = rec();
    r.update([t(1, 100, 100)], 0);
    expect(r.cancel()).toEqual([]);
    r.update([t(1, 0, 0), t(2, 100, 0)], 100);
    expect(r.cancel()).toEqual([]);
    // fresh after reset
    r.update([t(9, 10, 10)], 200);
    expect(kinds(r.update([], 250))).toEqual(["tap"]);
  });
});
