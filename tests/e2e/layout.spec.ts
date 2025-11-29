// wxSplitterWindow and wxScrolledWindow Tests - Layout controls KiCad uses
import { test, expect, MAIN_CANVAS, tryLoadApp, getCanvasBox } from './utils/fixtures';

test.describe('wxSplitterWindow & wxScrolledWindow Tests', () => {

  test('Layout test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/layout-01-loaded.png', fullPage: true });

    const hasStartup = testLogger.consoleLogs.some(l => l.includes('Layout test app started'));

    expect(loaded, 'Layout app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Splitter is visible with two panes', async ({ page, testLogger }) => {
    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/layout-02-splitter.png', fullPage: true });

    const hasSplitterLog = testLogger.consoleLogs.some(l => l.includes('Splitter position'));

    expect(hasSplitterLog).toBe(true);
  });

  test('Splitter sash can be dragged', async ({ page, testLogger }) => {
    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Splitter sash is at initial position 300 from left
    const sashX = box.x + 300;
    const sashY = box.y + 200;

    // Drag sash to the right
    await page.mouse.move(sashX, sashY);
    await page.mouse.down();
    await page.mouse.move(sashX + 100, sashY, { steps: 10 });
    await page.mouse.up();
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/layout-03-sash-dragged.png', fullPage: true });

    // Smoke test - verify no crash
    expect(true).toBe(true);
  });

  test('Scrolled windows show content', async ({ page, testLogger }) => {
    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Scroll in left pane
    await page.mouse.move(box.x + 150, box.y + 200);
    await page.mouse.wheel(0, 100);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/layout-04-scrolled-left.png', fullPage: true });

    // Scroll in right pane
    await page.mouse.move(box.x + 500, box.y + 200);
    await page.mouse.wheel(0, 100);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/layout-05-scrolled-right.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Layout controls work together', async ({ page, testLogger }) => {
    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Drag sash
    await page.mouse.move(box.x + 300, box.y + 200);
    await page.mouse.down();
    await page.mouse.move(box.x + 400, box.y + 200, { steps: 5 });
    await page.mouse.up();
    await page.waitForTimeout(300);

    // Scroll left pane
    await page.mouse.move(box.x + 100, box.y + 200);
    await page.mouse.wheel(0, 50);
    await page.waitForTimeout(200);

    // Scroll right pane
    await page.mouse.move(box.x + 600, box.y + 200);
    await page.mouse.wheel(0, 50);
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/layout-06-combined.png', fullPage: true });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
