import { describe, expect, it } from "vitest";
import { redirectTargetFor } from "./redirect";

const APP = "https://app.pcbjam.com";

describe("redirectTargetFor", () => {
  it("is off when no app URL is configured (dev / demo)", () => {
    expect(redirectTargetFor(null, "/")).toBeNull();
    expect(redirectTargetFor(null, "/alice/projects/board")).toBeNull();
    expect(redirectTargetFor(null, "/login")).toBeNull();
  });

  it("redirects home — the mgmt projects page is the real home", () => {
    expect(redirectTargetFor(APP, "/")).toBe(`${APP}/`);
  });

  it("redirects the project overview (3-segment path)", () => {
    expect(redirectTargetFor(APP, "/alice/projects/board")).toBe(
      `${APP}/alice/projects/board`,
    );
  });

  it("redirects mgmt-only paths (catch-all)", () => {
    for (const p of [
      "/login",
      "/libs",
      "/libs/import",
      "/alice",
      "/alice/members",
      "/admin/tasks",
      "/alice/projects",
    ]) {
      expect(redirectTargetFor(APP, p), p).toBe(APP + p);
    }
  });

  it("keeps editor surfaces local", () => {
    for (const p of [
      "/alice/projects/board/-/pcbnew",
      "/alice/projects/board/board.kicad_pcb",
      "/alice/projects/board/nested/dir/sub.kicad_sch",
      "/alice/libs/mylib",
    ]) {
      expect(redirectTargetFor(APP, p), p).toBeNull();
    }
  });

  it("keeps the browser-local pseudo-scope local (no mgmt counterpart)", () => {
    expect(redirectTargetFor(APP, "/@local/projects/scratch")).toBeNull();
    expect(redirectTargetFor(APP, "/%40local/projects/scratch")).toBeNull();
    expect(redirectTargetFor(APP, "/@local")).toBeNull();
  });

  it("preserves the query string", () => {
    expect(redirectTargetFor(APP, "/alice/projects/board", "?tab=files")).toBe(
      `${APP}/alice/projects/board?tab=files`,
    );
  });
});
