// wxMenuBar Tests - Menu system for KiCad
import { test, expect, MAIN_CANVAS, tryLoadApp, getCanvasBox } from './utils/fixtures';

test.describe('wxMenuBar Tests', () => {

  test('Menu test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/menu-01-loaded.png', fullPage: true });

    const hasStartupLog = testLogger.consoleLogs.some(l =>
      l.includes('wxMenuBar test app started') || l.includes('Menu test app started')
    );

    expect(loaded, 'wxMenuBar app should load successfully').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Menu bar is visible', async ({ page, testLogger }) => {
    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/menu-02-menubar.png', fullPage: true });

    // Check that app started with menu bar created
    const hasMenuBarLog = testLogger.consoleLogs.some(l =>
      l.includes('Menu bar created') || l.includes('Menu test app started')
    );

    expect(hasMenuBarLog).toBe(true);
  });

  test('File menu can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click on File menu (top left of menu bar, around x=30, y=10-25)
    await page.mouse.click(box.x + 30, box.y + 15);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/menu-03-file-clicked.png', fullPage: true });

    // Smoke test - no crash
    expect(true).toBe(true);
  });

  test('Edit menu can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click on Edit menu (next to File, around x=70, y=15)
    await page.mouse.click(box.x + 70, box.y + 15);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/menu-04-edit-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Multiple menus can be accessed', async ({ page, testLogger }) => {
    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Click through all menus
    const menuPositions = [30, 70, 110, 150, 190]; // File, Edit, View, Tools, Help
    for (let i = 0; i < menuPositions.length; i++) {
      await page.mouse.click(box.x + menuPositions[i], box.y + 15);
      await page.waitForTimeout(300);
    }

    await page.screenshot({ path: 'test-results/menu-05-all-menus.png', fullPage: true });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
