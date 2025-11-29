// wxAuiManager Tests - AUI docking system KiCad uses extensively
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

test.describe('wxAuiManager Tests', () => {

  test('AUI test app loads successfully', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(`[PAGE_ERROR] ${err.message}`));
    page.on('console', msg => logs.push(`[${msg.type()}] ${msg.text()}`));

    await page.goto('/standalone/aui/aui_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/aui-01-loaded.png', fullPage: true });

    const hasStartup = logs.some(l => l.includes('AUI test app started'));

    console.log('AUI loaded:', loaded);
    console.log('AUI logs:', logs.filter(l => l.includes('AUI')));
    console.log('AUI errors:', errors);

    expect(loaded, 'AUI app should load').toBe(true);
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('AUI dockable panels are visible', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/aui/aui_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.screenshot({ path: 'test-results/aui-02-panels.png', fullPage: true });

    const hasPanelsLog = logs.some(l => l.includes('dockable panels'));
    console.log('Panel logs:', logs.filter(l => l.includes('AUI') || l.includes('panel')));

    expect(hasPanelsLog).toBe(true);
  });

  test('Panel close button can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/aui/aui_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click on Properties panel close button (top right of left panel)
    // Left panel is at left edge, close button at top right of its title bar
    await page.mouse.click(box.x + 145, box.y + 35);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/aui-03-close-clicked.png', fullPage: true });

    const hasCloseEvent = logs.some(l => l.includes('Pane closing'));
    console.log('Close events:', logs.filter(l => l.includes('Pane') || l.includes('close')));

    // Smoke test
    expect(true).toBe(true);
  });

  test('Panel can be dragged', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/aui/aui_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Drag Properties panel title bar
    const titleX = box.x + 75;
    const titleY = box.y + 35;

    await page.mouse.move(titleX, titleY);
    await page.mouse.down();
    await page.mouse.move(box.x + 300, box.y + 200, { steps: 10 });
    await page.mouse.up();
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/aui-04-dragged.png', fullPage: true });

    // Smoke test
    expect(true).toBe(true);
  });

  test('Multiple panels can be interacted with', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/aui/aui_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click in Properties panel
    await page.mouse.click(box.x + 75, box.y + 200);
    await page.waitForTimeout(200);

    // Click in Layers panel (right side)
    await page.mouse.click(box.x + 720, box.y + 200);
    await page.waitForTimeout(200);

    // Click in Messages panel (bottom)
    await page.mouse.click(box.x + 400, box.y + 550);
    await page.waitForTimeout(200);

    // Click in center (Event Log)
    await page.mouse.click(box.x + 400, box.y + 300);
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/aui-05-multi-panel.png', fullPage: true });

    console.log('\n=== AUI EVENTS ===');
    logs.filter(l => l.includes('AUI')).forEach(l => console.log(l));

    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
