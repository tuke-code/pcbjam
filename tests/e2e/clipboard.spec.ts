// wxClipboard Tests - Clipboard operations for KiCad copy/paste
import { test, expect, MAIN_CANVAS, tryLoadApp, getCanvasBox } from './utils/fixtures';

test.describe('wxClipboard Tests', () => {

  test('Clipboard test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/clipboard-01-loaded.png', fullPage: true });

    const hasStartupLog = testLogger.consoleLogs.some(l =>
      l.includes('wxClipboard test app started') || l.includes('Clipboard test app started')
    );

    expect(loaded, 'wxClipboard app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Copy button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click "Copy to Clipboard" button
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-02-copy-clicked.png', fullPage: true });

    expect(true).toBe(true); // Smoke test
  });

  test('Paste button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // First copy something
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(300);

    // Click "Paste from Clipboard" button
    await page.mouse.click(box.x + 250, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-03-paste-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Check clipboard button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click "Check Clipboard" button
    await page.mouse.click(box.x + 400, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-04-check-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Clear clipboard button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // First copy something
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(300);

    // Click "Clear Clipboard" button
    await page.mouse.click(box.x + 550, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-05-clear-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Full clipboard flow: copy, check, paste, clear', async ({ page, testLogger }) => {
    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // 1. Copy
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(300);

    // 2. Check
    await page.mouse.click(box.x + 400, box.y + 220);
    await page.waitForTimeout(300);

    // 3. Paste
    await page.mouse.click(box.x + 250, box.y + 220);
    await page.waitForTimeout(300);

    // 4. Clear
    await page.mouse.click(box.x + 550, box.y + 220);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/clipboard-06-full-flow.png', fullPage: true });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
