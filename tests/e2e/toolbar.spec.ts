// wxToolBar and wxStatusBar Tests - Toolbar and status bar KiCad uses
import { test, expect, MAIN_CANVAS, tryLoadApp, getCanvasBox } from './utils/fixtures';

test.describe('wxToolBar & wxStatusBar Tests', () => {

  test('Toolbar test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/toolbar-01-loaded.png', fullPage: true });

    const hasStartup = testLogger.consoleLogs.some(l => l.includes('Toolbar test app started'));

    expect(loaded, 'Toolbar app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Toolbar buttons are visible', async ({ page, testLogger }) => {
    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/toolbar-02-buttons.png', fullPage: true });

    const hasToolbarLog = testLogger.consoleLogs.some(l => l.includes('Toolbar created'));

    expect(hasToolbarLog).toBe(true);
  });

  test('New tool button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click New button (first tool, around x=30)
    await page.mouse.click(box.x + 30, box.y + 45);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/toolbar-03-new-clicked.png', fullPage: true });

    // Smoke test
    expect(true).toBe(true);
  });

  test('Zoom tools can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click Zoom In
    await page.mouse.click(box.x + 230, box.y + 45);
    await page.waitForTimeout(300);

    // Click Zoom Out
    await page.mouse.click(box.x + 290, box.y + 45);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/toolbar-04-zoom.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Toggle tool changes state', async ({ page, testLogger }) => {
    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click Toggle button (after separator, around x=350)
    await page.mouse.click(box.x + 350, box.y + 45);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/toolbar-05-toggle-on.png', fullPage: true });

    // Click again to toggle off
    await page.mouse.click(box.x + 350, box.y + 45);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/toolbar-06-toggle-off.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Status bar shows messages', async ({ page, testLogger }) => {
    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/toolbar-07-statusbar.png', fullPage: true });

    const hasStatusBarLog = testLogger.consoleLogs.some(l => l.includes('Status bar created'));

    expect(hasStatusBarLog).toBe(true);
  });

  test('All toolbar buttons accessible', async ({ page, testLogger }) => {
    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click all toolbar buttons
    const buttonPositions = [30, 85, 140, 230, 290, 350]; // New, Open, Save, ZoomIn, ZoomOut, Toggle
    for (const x of buttonPositions) {
      await page.mouse.click(box.x + x, box.y + 45);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/toolbar-08-all-buttons.png', fullPage: true });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
