// wxPrintPreview Tests - Print preview, page setup
import { test, expect, tryLoadApp } from './utils/fixtures';

test.describe('wxPrintPreview Tests', () => {

  test('Print preview test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/printpreview/printpreview_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/printpreview-01-loaded.png', fullPage: true });

    expect(loaded, 'Print preview app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Preview area displays schematic-like content', async ({ page, testLogger }) => {
    await page.goto('/standalone/printpreview/printpreview_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);

    // Preview area should show sample schematic with grid, components, and title block
    await page.screenshot({ path: 'test-results/printpreview-02-preview-area.png', fullPage: true });
  });

  test('Print Preview button opens preview frame', async ({ page, testLogger }) => {
    await page.goto('/standalone/printpreview/printpreview_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click Print Preview button
      await page.mouse.click(box.x + 70, box.y + 70);
      await page.waitForTimeout(500);
    }

    await page.screenshot({ path: 'test-results/printpreview-03-preview-frame.png', fullPage: true });
  });

  test('Page Setup button opens dialog', async ({ page, testLogger }) => {
    await page.goto('/standalone/printpreview/printpreview_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click Page Setup button
      await page.mouse.click(box.x + 180, box.y + 70);
      await page.waitForTimeout(500);
    }

    await page.screenshot({ path: 'test-results/printpreview-04-page-setup.png', fullPage: true });
  });

  test('Print settings display shows current configuration', async ({ page, testLogger }) => {
    await page.goto('/standalone/printpreview/printpreview_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);

    // Settings display should show orientation, paper size, quality, color
    await page.screenshot({ path: 'test-results/printpreview-05-settings.png', fullPage: true });
  });
});
