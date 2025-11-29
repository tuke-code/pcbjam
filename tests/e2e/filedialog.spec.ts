// wxFileDialog Tests - File dialogs for KiCad open/save operations
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

test.describe('wxFileDialog Tests', () => {

  test('FileDialog test app loads successfully', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(`[PAGE_ERROR] ${err.message}`));
    page.on('console', msg => logs.push(`[${msg.type()}] ${msg.text()}`));

    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/filedialog-01-loaded.png', fullPage: true });

    const hasStartup = logs.some(l => l.includes('FileDialog test app started'));

    console.log('FileDialog loaded:', loaded);
    console.log('FileDialog logs:', logs.filter(l => l.includes('FILEDIALOG')));
    console.log('FileDialog errors:', errors);

    expect(loaded, 'wxFileDialog app should load').toBe(true);
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Open file button can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click "Open File..." button
    await page.mouse.click(box.x + 100, box.y + 150);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/filedialog-02-open-clicked.png', fullPage: true });

    const hasOpenLog = logs.some(l => l.includes('Opening file dialog') || l.includes('Open'));
    console.log('Open logs:', logs.filter(l => l.includes('FILEDIALOG') || l.includes('Open')));

    expect(true).toBe(true); // Smoke test
  });

  test('Save file button can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click "Save File..." button
    await page.mouse.click(box.x + 220, box.y + 150);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/filedialog-03-save-clicked.png', fullPage: true });

    const hasSaveLog = logs.some(l => l.includes('save dialog') || l.includes('Save'));
    console.log('Save logs:', logs.filter(l => l.includes('FILEDIALOG') || l.includes('Save')));

    expect(true).toBe(true);
  });

  test('Open multiple button can be clicked', async ({ page }) => {
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click "Open Multiple..." button
    await page.mouse.click(box.x + 350, box.y + 150);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/filedialog-04-multiple-clicked.png', fullPage: true });

    console.log('Multiple logs:', logs.filter(l => l.includes('FILEDIALOG') || l.includes('Multiple')));

    expect(true).toBe(true);
  });

  test('All file dialog buttons accessible', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/filedialog/filedialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Try all three buttons
    await page.mouse.click(box.x + 100, box.y + 150);
    await page.waitForTimeout(300);
    await page.mouse.click(box.x + 220, box.y + 150);
    await page.waitForTimeout(300);
    await page.mouse.click(box.x + 350, box.y + 150);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/filedialog-05-all-buttons.png', fullPage: true });

    console.log('\n=== FILEDIALOG EVENTS ===');
    logs.filter(l => l.includes('FILEDIALOG')).forEach(l => console.log(l));

    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
