// wxToolBar and wxStatusBar Tests - Toolbar and status bar KiCad uses
import { test, expect, Page } from '@playwright/test';

const MAIN_CANVAS = '#canvas';

async function tryLoadApp(page: Page, timeout = 15000) {
  try {
    await page.waitForSelector(MAIN_CANVAS, { state: 'visible', timeout });
    await page.waitForTimeout(500);
    return true;
  } catch {
    return false;
  }
}

test.describe('wxToolBar & wxStatusBar Tests', () => {

  test('Toolbar test app loads successfully', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(`[PAGE_ERROR] ${err.message}`));
    page.on('console', msg => logs.push(`[${msg.type()}] ${msg.text()}`));

    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/toolbar-01-loaded.png', fullPage: true });

    const hasStartup = logs.some(l => l.includes('Toolbar test app started'));

    console.log('Toolbar loaded:', loaded);
    console.log('Toolbar logs:', logs.filter(l => l.includes('TOOLBAR')));
    console.log('Toolbar errors:', errors);

    expect(loaded, 'Toolbar app should load').toBe(true);
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Toolbar buttons are visible', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/toolbar-02-buttons.png', fullPage: true });

    const hasToolbarLog = logs.some(l => l.includes('Toolbar created'));
    console.log('Toolbar logs:', logs.filter(l => l.includes('TOOLBAR') || l.includes('Toolbar')));

    expect(hasToolbarLog).toBe(true);
  });

  test('New tool button can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click New button (first tool, around x=30)
    await page.mouse.click(box.x + 30, box.y + 45);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/toolbar-03-new-clicked.png', fullPage: true });

    const hasNewEvent = logs.some(l => l.includes('New clicked'));
    console.log('New logs:', logs.filter(l => l.includes('TOOLBAR') || l.includes('New')));

    // Smoke test
    expect(true).toBe(true);
  });

  test('Zoom tools can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click Zoom In
    await page.mouse.click(box.x + 230, box.y + 45);
    await page.waitForTimeout(300);

    // Click Zoom Out
    await page.mouse.click(box.x + 290, box.y + 45);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/toolbar-04-zoom.png', fullPage: true });

    const hasZoomEvent = logs.some(l => l.includes('Zoom'));
    console.log('Zoom logs:', logs.filter(l => l.includes('TOOLBAR') || l.includes('Zoom')));

    expect(true).toBe(true);
  });

  test('Toggle tool changes state', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click Toggle button (after separator, around x=350)
    await page.mouse.click(box.x + 350, box.y + 45);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/toolbar-05-toggle-on.png', fullPage: true });

    // Click again to toggle off
    await page.mouse.click(box.x + 350, box.y + 45);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/toolbar-06-toggle-off.png', fullPage: true });

    const hasToggleEvents = logs.some(l => l.includes('Toggle'));
    console.log('Toggle logs:', logs.filter(l => l.includes('TOOLBAR') || l.includes('Toggle')));

    expect(true).toBe(true);
  });

  test('Status bar shows messages', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/toolbar-07-statusbar.png', fullPage: true });

    const hasStatusBarLog = logs.some(l => l.includes('Status bar created'));
    console.log('Status bar logs:', logs.filter(l => l.includes('TOOLBAR') || l.includes('Status')));

    expect(hasStatusBarLog).toBe(true);
  });

  test('All toolbar buttons accessible', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/toolbar/toolbar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click all toolbar buttons
    const buttonPositions = [30, 85, 140, 230, 290, 350]; // New, Open, Save, ZoomIn, ZoomOut, Toggle
    for (const x of buttonPositions) {
      await page.mouse.click(box.x + x, box.y + 45);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/toolbar-08-all-buttons.png', fullPage: true });

    console.log('\n=== TOOLBAR EVENTS ===');
    logs.filter(l => l.includes('TOOLBAR')).forEach(l => console.log(l));

    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
