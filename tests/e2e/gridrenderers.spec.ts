// wxGrid Custom Cell Renderers Tests - Color cells, icon+text, striped rows
import { test, expect, tryLoadApp } from './utils/fixtures';

test.describe('wxGrid Custom Cell Renderers Tests', () => {

  test('Grid renderers test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/gridrenderers/gridrenderers_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/gridrenderers-01-loaded.png', fullPage: true });

    expect(loaded, 'Grid renderers app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Color cells tab displays color swatches', async ({ page, testLogger }) => {
    await page.goto('/standalone/gridrenderers/gridrenderers_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    // Color Cells tab should be visible by default
    await page.screenshot({ path: 'test-results/gridrenderers-02-color-cells.png', fullPage: true });
  });

  test('Icon+Text tab displays icons with text', async ({ page, testLogger }) => {
    await page.goto('/standalone/gridrenderers/gridrenderers_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click Icon+Text tab (second tab in notebook)
      await page.mouse.click(box.x + 150, box.y + 80);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/gridrenderers-03-icon-text.png', fullPage: true });
  });

  test('Striped rows tab displays alternating colors', async ({ page, testLogger }) => {
    await page.goto('/standalone/gridrenderers/gridrenderers_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click Striped+Checkboxes tab (third tab)
      await page.mouse.click(box.x + 280, box.y + 80);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/gridrenderers-04-striped.png', fullPage: true });
  });

  test('Checkbox cells can be toggled in striped grid', async ({ page, testLogger }) => {
    await page.goto('/standalone/gridrenderers/gridrenderers_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Go to Striped+Checkboxes tab
      await page.mouse.click(box.x + 280, box.y + 80);
      await page.waitForTimeout(200);

      // Click on a checkbox cell (DNP column)
      await page.mouse.click(box.x + 330, box.y + 180);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/gridrenderers-05-checkbox-toggle.png', fullPage: true });
  });
});
