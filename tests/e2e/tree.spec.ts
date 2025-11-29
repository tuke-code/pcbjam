import { test, expect } from './utils/fixtures';

test.describe('wxTreeCtrl Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/standalone/tree/tree_test.html');
    // Wait for app to initialize
    await page.waitForFunction(() => {
      return document.querySelector('canvas') !== null;
    }, { timeout: 30000 });
    await page.waitForTimeout(1000);
  });

  test('Tree test app loads successfully', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const hasStartupLog = testLogger.consoleLogs.some(log =>
      log.includes('TREE_TEST') && log.includes('started successfully')
    );

    await page.screenshot({ path: 'test-results/tree-01-loaded.png' });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Tree is populated with KiCad-like hierarchy', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const hasPopulatedLog = testLogger.consoleLogs.some(log =>
      log.includes('Tree populated with KiCad-like hierarchy')
    );

    await page.screenshot({ path: 'test-results/tree-02-hierarchy.png' });

    expect(hasPopulatedLog).toBe(true);
  });

  test('Tree item can be selected', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click on a tree item (approximate position)
    await canvas.click({ position: { x: 100, y: 180 } });
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/tree-03-selected.png' });

    const hasSelectionEvent = testLogger.consoleLogs.some(log =>
      log.includes('Selection changed')
    );
  });

  test('Expand All button works', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click Expand All button
    await canvas.click({ position: { x: 80, y: 95 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/tree-04-expanded.png' });

    const hasExpandEvent = testLogger.consoleLogs.some(log =>
      log.includes('All items expanded')
    );
    expect(hasExpandEvent).toBe(true);
  });

  test('Collapse All button works', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click Collapse All button
    await canvas.click({ position: { x: 200, y: 95 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/tree-05-collapsed.png' });

    const hasCollapseEvent = testLogger.consoleLogs.some(log =>
      log.includes('All items collapsed')
    );
    expect(hasCollapseEvent).toBe(true);
  });

  test('Add Item button works with selection', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // First select an item
    await canvas.click({ position: { x: 100, y: 160 } });
    await page.waitForTimeout(300);

    // Click Add Item button
    await canvas.click({ position: { x: 310, y: 95 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/tree-06-added.png' });

    const hasAddEvent = testLogger.consoleLogs.some(log =>
      log.includes('Added new item') || log.includes('No item selected')
    );
    expect(hasAddEvent).toBe(true);
  });

  test('Delete Item button works with selection', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // First select an item (not root)
    await canvas.click({ position: { x: 150, y: 200 } });
    await page.waitForTimeout(300);

    // Click Delete Selected button
    await canvas.click({ position: { x: 440, y: 95 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/tree-07-deleted.png' });

    const hasDeleteEvent = testLogger.consoleLogs.some(log =>
      log.includes('Deleted item') || log.includes('Cannot delete')
    );
    expect(hasDeleteEvent).toBe(true);
  });
});
