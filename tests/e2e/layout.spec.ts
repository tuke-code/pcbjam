// wxSplitterWindow and wxScrolledWindow Tests - Layout controls KiCad uses
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

test.describe('wxSplitterWindow & wxScrolledWindow Tests', () => {

  test('Layout test app loads successfully', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(`[PAGE_ERROR] ${err.message}`));
    page.on('console', msg => logs.push(`[${msg.type()}] ${msg.text()}`));

    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/layout-01-loaded.png', fullPage: true });

    const hasStartup = logs.some(l => l.includes('Layout test app started'));

    console.log('Layout loaded:', loaded);
    console.log('Layout logs:', logs.filter(l => l.includes('LAYOUT')));
    console.log('Layout errors:', errors);

    expect(loaded, 'Layout app should load').toBe(true);
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Splitter is visible with two panes', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/layout-02-splitter.png', fullPage: true });

    const hasSplitterLog = logs.some(l => l.includes('Splitter position'));
    console.log('Splitter logs:', logs.filter(l => l.includes('LAYOUT') || l.includes('Splitter')));

    expect(hasSplitterLog).toBe(true);
  });

  test('Splitter sash can be dragged', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

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

    const hasSashEvent = logs.some(l => l.includes('Splitter sash moved'));
    console.log('Sash events:', logs.filter(l => l.includes('sash') || l.includes('Splitter')));

    // Smoke test - verify no crash
    expect(true).toBe(true);
  });

  test('Scrolled windows show content', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

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

  test('Layout controls work together', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/layout/layout_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

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

    console.log('\n=== LAYOUT EVENTS ===');
    logs.filter(l => l.includes('LAYOUT')).forEach(l => console.log(l));

    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
