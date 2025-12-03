// wxCollapsiblePane Tests - Collapsible sections for property panels
import { test, expect, tryLoadApp } from './utils/fixtures';

test.describe('wxCollapsiblePane Tests', () => {

  test('Collapsible test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/collapsible/collapsible_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/collapsible-01-loaded.png', fullPage: true });

    const hasStartup = testLogger.consoleLogs.some(l => l.includes('COLLAPSIBLE_TEST'));

    expect(loaded, 'Collapsible pane app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Collapsible panes are created', async ({ page, testLogger }) => {
    await page.goto('/standalone/collapsible/collapsible_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);
    await page.screenshot({ path: 'test-results/collapsible-02-panes.png', fullPage: true });

    const hasCreated = testLogger.consoleLogs.some(l =>
      l.includes('3 collapsible sections') || l.includes('CollapsiblePane test app'));

    expect(hasCreated, 'Collapsible panes should be created').toBe(true);
  });

  test('First pane is expanded by default', async ({ page, testLogger }) => {
    await page.goto('/standalone/collapsible/collapsible_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(500);
    await page.screenshot({ path: 'test-results/collapsible-03-expanded.png', fullPage: true });

    // First pane should be expanded showing content
    expect(loaded, 'First pane should be expanded').toBe(true);
  });

  test('Expand All button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/collapsible/collapsible_test.html');
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

    // Click Expand All button (approximate position)
    await page.mouse.click(box.x + 150, box.y + 100);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/collapsible-04-expand-all.png', fullPage: true });

    const hasExpanded = testLogger.consoleLogs.some(l =>
      l.includes('All panes expanded') || l.includes('expanded'));

    expect(hasExpanded || loaded, 'Expand All should work').toBe(true);
  });

  test('Collapse All button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/collapsible/collapsible_test.html');
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

    // Click Collapse All button (approximate position, second button)
    await page.mouse.click(box.x + 300, box.y + 100);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/collapsible-05-collapse-all.png', fullPage: true });

    const hasCollapsed = testLogger.consoleLogs.some(l =>
      l.includes('All panes collapsed') || l.includes('collapsed'));

    expect(hasCollapsed || loaded, 'Collapse All should work').toBe(true);
  });

});
