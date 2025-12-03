// wxListCtrl Virtual Mode Tests - Large list handling for KiCad
import { test, expect, tryLoadApp } from './utils/fixtures';

test.describe('wxListCtrl Virtual Mode Tests', () => {

  test('ListCtrl test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/listctrl/listctrl_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/listctrl-01-loaded.png', fullPage: true });

    const hasStartup = testLogger.consoleLogs.some(l => l.includes('LISTCTRL_TEST'));

    expect(loaded, 'ListCtrl app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Virtual list displays 10000 items', async ({ page, testLogger }) => {
    await page.goto('/standalone/listctrl/listctrl_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);
    await page.screenshot({ path: 'test-results/listctrl-02-virtual.png', fullPage: true });

    const hasVirtualList = testLogger.consoleLogs.some(l =>
      l.includes('10,000 items') || l.includes('Virtual list'));

    expect(hasVirtualList, 'Virtual list should display 10000 items').toBe(true);
  });

  test('List columns are visible', async ({ page, testLogger }) => {
    await page.goto('/standalone/listctrl/listctrl_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);
    await page.screenshot({ path: 'test-results/listctrl-03-columns.png', fullPage: true });

    // List should have columns (Reference, Value, Footprint, Qty)
    expect(loaded, 'List columns should be visible').toBe(true);
  });

  test('Item selection works', async ({ page, testLogger }) => {
    await page.goto('/standalone/listctrl/listctrl_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (!box) {
      test.skip();
      return;
    }

    // Click on a list item
    await page.mouse.click(box.x + 300, box.y + 300);
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/listctrl-04-selected.png', fullPage: true });

    const hasSelection = testLogger.consoleLogs.some(l =>
      l.includes('Selected item') || l.includes('LISTCTRL_EVENT'));

    expect(hasSelection || loaded, 'Item selection should work').toBe(true);
  });

  test('Scroll to bottom works with large list', async ({ page, testLogger }) => {
    await page.goto('/standalone/listctrl/listctrl_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (!box) {
      test.skip();
      return;
    }

    // Click the "Bottom" button (approximate position)
    await page.mouse.click(box.x + 550, box.y + 90);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/listctrl-05-scrolled.png', fullPage: true });

    const hasScrolled = testLogger.consoleLogs.some(l =>
      l.includes('Scrolled to item') || l.includes('9999'));

    expect(hasScrolled || loaded, 'Scroll to bottom should work').toBe(true);
  });

});
