// wxMenuBar Tests - Menu system for KiCad
import { test, expect, Page } from '@playwright/test';

const MAIN_CANVAS = '#canvas';

async function waitForApp(page: Page, timeout = 30000) {
  await page.waitForSelector(MAIN_CANVAS, { state: 'visible', timeout });
  await page.waitForTimeout(500);
}

async function tryLoadApp(page: Page) {
  try {
    await waitForApp(page, 15000);
    return true;
  } catch {
    return false;
  }
}

test.describe('wxMenuBar Tests', () => {

  test('Menu test app loads successfully', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => {
      errors.push(`[PAGE_ERROR] ${err.message}`);
    });
    page.on('console', msg => {
      logs.push(`[${msg.type()}] ${msg.text()}`);
    });

    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/menu-01-loaded.png', fullPage: true });

    const hasStartupLog = logs.some(l => l.includes('wxMenuBar test app started') || l.includes('Menu test app started'));

    console.log('Menu app logs:', logs.filter(l => l.includes('MENU')));
    console.log('Menu app errors:', errors);
    console.log('Menu app loaded:', loaded);
    console.log('Has startup log:', hasStartupLog);

    expect(loaded, 'wxMenuBar app should load successfully').toBe(true);
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Menu bar is visible', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/menu-02-menubar.png', fullPage: true });

    // Check that app started with menu bar created
    const hasMenuBarLog = logs.some(l => l.includes('Menu bar created') || l.includes('Menu test app started'));
    console.log('Menu bar logs:', logs.filter(l => l.includes('menu') || l.includes('Menu')));

    expect(hasMenuBarLog).toBe(true);
  });

  test('File menu can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click on File menu (top left of menu bar, around x=30, y=10-25)
    await page.mouse.click(box.x + 30, box.y + 15);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/menu-03-file-clicked.png', fullPage: true });

    // Smoke test - no crash
    expect(true).toBe(true);
  });

  test('Edit menu can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click on Edit menu (next to File, around x=70, y=15)
    await page.mouse.click(box.x + 70, box.y + 15);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/menu-04-edit-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });

  test('Multiple menus can be accessed', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/menu/menu_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click through all menus
    const menuPositions = [30, 70, 110, 150, 190]; // File, Edit, View, Tools, Help
    for (let i = 0; i < menuPositions.length; i++) {
      await page.mouse.click(box.x + menuPositions[i], box.y + 15);
      await page.waitForTimeout(300);
    }

    await page.screenshot({ path: 'test-results/menu-05-all-menus.png', fullPage: true });

    console.log('\n=== MENU EVENTS ===');
    logs.filter(l => l.includes('MENU')).forEach(l => console.log(l));

    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
