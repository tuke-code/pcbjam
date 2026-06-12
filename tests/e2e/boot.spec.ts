import { test, expect, waitForApp } from './utils/fixtures';

// Boot check: the app starts, the wx element registry fills, and the
// wx-dom shim is present. Intentionally takes no screenshots.
test.describe('Application boot', () => {
  test('minimal app boots and registers elements', async ({ page, testLogger }) => {
    const fatal: string[] = [];
    page.on('pageerror', (err) => fatal.push(String(err)));

    await page.goto('/minimal_test.html');
    await waitForApp(page);

    // The registry fills as wx windows are created.
    await page.waitForFunction(
      () => (window as any).wxElementRegistry?.elements?.size > 0,
      undefined,
      { timeout: 30000 }
    );

    const elementCount = await page.evaluate(
      () => (window as any).wxElementRegistry.elements.size
    );
    expect(elementCount).toBeGreaterThan(0);

    const isDomPort = await page.evaluate(() => (window as any).wxDomPort === true);
    expect(isDomPort, 'wx-dom.js shim should be loaded').toBe(true);

    expect(fatal, `page errors: ${fatal.join('\n')}`).toHaveLength(0);
  });
});
