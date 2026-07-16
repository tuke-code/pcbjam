// @vitest-environment happy-dom
import * as fs from "node:fs";
import * as path from "node:path";
import { fileURLToPath } from "node:url";
import { afterEach, describe, expect, it, vi } from "vitest";

/**
 * boot.js honors the ?base= / ?cdn= / ?tag= script-source overrides only on a
 * loopback hostname; a shipped page must ignore them. Black-box test: run the
 * REAL boot.js in a DOM at a given URL and check whether an external-origin
 * <script> gets injected.
 */
const bootSrc = fs.readFileSync(
  path.join(path.dirname(fileURLToPath(import.meta.url)), "../public/gerber-demo/boot.js"),
  "utf8",
);

const EXTERNAL = "https://external.example/x";

async function runBootAt(url: string): Promise<void> {
  const hd = (window as unknown as {
    happyDOM: { setURL(u: string): void; settings: { disableJavaScriptFileLoading: boolean } };
  }).happyDOM;
  // We assert on the injected <script src>, not on it actually loading — keep
  // happy-dom from fetching the URL over the network.
  hd.settings.disableJavaScriptFileLoading = true;
  hd.setURL(url);
  // The status scaffold boot.js's helpers look up (all null-guarded anyway).
  document.body.innerHTML =
    '<div id="status"><span id="status-text"></span><div id="progress-bar"></div></div>' +
    '<div id="main-window"></div>';
  // The default path fetches a CDN manifest — let it fail; we only assert on
  // script injection, never on a successful boot.
  vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("no network in test")));
  (0, eval)(bootSrc);
  await new Promise((r) => setTimeout(r, 20)); // let resolveBase().then(boot) settle
}

function externalScriptsInjected(): string[] {
  return Array.from(document.querySelectorAll("script[src]"))
    .map((s) => s.getAttribute("src") || "")
    .filter((src) => /^https?:\/\//i.test(src));
}

describe("gerber-demo boot.js override gate", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
    document.body.innerHTML = "";
    document.querySelectorAll("script[src]").forEach((s) => s.remove());
  });

  it("ignores ?base= on a production origin — no external script injected", async () => {
    await runBootAt(
      "https://www.pcbjam.com/gerber-demo/index.html?base=" + encodeURIComponent(EXTERNAL),
    );
    expect(externalScriptsInjected().some((s) => s.includes("external.example"))).toBe(false);
  });

  it("still honors ?base= on localhost — the dev override is preserved", async () => {
    await runBootAt(
      "http://localhost:4321/gerber-demo/index.html?base=" + encodeURIComponent(EXTERNAL),
    );
    expect(externalScriptsInjected().some((s) => s.includes("external.example"))).toBe(true);
  });
});
