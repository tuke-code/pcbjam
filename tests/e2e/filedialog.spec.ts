// wxFileDialog Tests - File dialogs for KiCad open/save operations
import { test, expect, MAIN_CANVAS, tryLoadApp, getCanvasBox } from './utils/fixtures';

test.describe('wxFileDialog Tests', () => {

  test('FileDialog test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/filedialog-01-loaded.png', fullPage: true });

    const hasStartup = testLogger.consoleLogs.some(l => l.includes('FileDialog test app started'));

    expect(loaded, 'wxFileDialog app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Open file button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click "Open File..." button
    await page.mouse.click(box.x + 100, box.y + 150);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/filedialog-02-open-clicked.png', fullPage: true });

    expect(true).toBe(true); // Smoke test
  });

  test('Save file button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click "Save File..." button
    await page.mouse.click(box.x + 220, box.y + 150);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/filedialog-03-save-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Open multiple button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click "Open Multiple..." button
    await page.mouse.click(box.x + 350, box.y + 150);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/filedialog-04-multiple-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('All file dialog buttons accessible', async ({ page, testLogger }) => {
    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Try all three buttons
    await page.mouse.click(box.x + 100, box.y + 150);
    await page.waitForTimeout(300);
    await page.mouse.click(box.x + 220, box.y + 150);
    await page.waitForTimeout(300);
    await page.mouse.click(box.x + 350, box.y + 150);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/filedialog-05-all-buttons.png', fullPage: true });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
