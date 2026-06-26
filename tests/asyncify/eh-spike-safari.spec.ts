import { test, expect } from '@playwright/test';

// Minimal native wasm-EH reproduction for the Safari/WebKit -fwasm-exceptions startup-crash issue
// (emscripten #25365). Loads apps/safari-eh/{legacy,new}.html (built by apps/safari-eh/build.sh) on
// every engine. On a healthy engine main() throws + catches and sets window.__ehResult="EH_OK code=42";
// on the Safari regression the module fails to instantiate (window.__ehError set) or hard-crashes
// (neither set -> our 15s timeout fallback). Run all 3 engines — the point is the webkit (Safari) row.
for (const variant of ['legacy', 'new'] as const) {
  test(`native wasm-EH: ${variant} encoding instantiates + catches`, async ({ page }, testInfo) => {
    const pageErrors: string[] = [];
    page.on('pageerror', (e) => pageErrors.push(e.message));

    await page.goto(`/safari-eh/${variant}.html`);

    const res = await page
      .waitForFunction(() => (window as any).__ehResult || (window as any).__ehError, null, { timeout: 15000 })
      .then((h) => h.jsonValue())
      .catch(() => '(timeout — neither __ehResult nor __ehError set; likely a hard engine crash)');

    console.log(`[SAFARI-EH ${testInfo.project.name}/${variant}] ${JSON.stringify(res)}  pageErrors=${JSON.stringify(pageErrors)}`);
    expect(String(res), `native-EH ${variant} must instantiate + catch on ${testInfo.project.name}`).toContain('EH_OK');
  });
}
