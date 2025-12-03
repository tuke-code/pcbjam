// Specialized wxWidgets Controls Tests - Treebook, RearrangeCtrl, BitmapComboBox
import { test, expect, tryLoadApp } from './utils/fixtures';

test.describe('Specialized wxWidgets Controls Tests', () => {

  test('Specialized controls test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/specialized-01-loaded.png', fullPage: true });

    expect(loaded, 'Specialized controls app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('wxTreebook displays tree with pages', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);

    // Treebook should show General page by default with tree on left
    await page.screenshot({ path: 'test-results/specialized-02-treebook-initial.png', fullPage: true });
  });

  test('wxTreebook can navigate to sub-pages', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click on Display > Colors sub-page in tree
      await page.mouse.click(box.x + 80, box.y + 180);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/specialized-03-treebook-subpage.png', fullPage: true });
  });

  test('wxTreebook can expand tree nodes', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click on Editing page
      await page.mouse.click(box.x + 80, box.y + 220);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/specialized-04-treebook-expand.png', fullPage: true });
  });

  test('wxBitmapComboBox displays layer swatches', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click on layer combo box to open dropdown
      await page.mouse.click(box.x + 620, box.y + 110);
      await page.waitForTimeout(300);
    }

    await page.screenshot({ path: 'test-results/specialized-05-bitmapcombo-dropdown.png', fullPage: true });
  });

  test('wxBitmapComboBox can select different layer', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Open combo dropdown
      await page.mouse.click(box.x + 620, box.y + 110);
      await page.waitForTimeout(200);
      // Select B.Cu layer
      await page.mouse.click(box.x + 620, box.y + 180);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/specialized-06-bitmapcombo-select.png', fullPage: true });
  });

  test('wxRearrangeCtrl displays layer order', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);

    // RearrangeCtrl should show list of layers with checkboxes
    await page.screenshot({ path: 'test-results/specialized-07-rearrange-list.png', fullPage: true });
  });

  test('wxRearrangeCtrl Get Layer Order button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/specialized/specialized_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click Get Layer Order button
      await page.mouse.click(box.x + 620, box.y + 480);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/specialized-08-get-order.png', fullPage: true });
  });
});
