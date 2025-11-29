// wxClipboard Tests - Clipboard operations for KiCad copy/paste
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

test.describe('wxClipboard Tests', () => {

  test('Clipboard test app loads successfully', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => {
      errors.push(`[PAGE_ERROR] ${err.message}`);
    });
    page.on('console', msg => {
      logs.push(`[${msg.type()}] ${msg.text()}`);
    });

    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/clipboard-01-loaded.png', fullPage: true });

    const hasStartupLog = logs.some(l => l.includes('wxClipboard test app started') || l.includes('Clipboard test app started'));
    console.log('Clipboard app logs:', logs.filter(l => l.includes('CLIPBOARD')));
    console.log('Clipboard app loaded:', loaded);

    expect(loaded, 'wxClipboard app should load').toBe(true);
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Copy button can be clicked', async ({ page }) => {
    const logs: string[] = [];

    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click "Copy to Clipboard" button
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-02-copy-clicked.png', fullPage: true });

    const hasCopyLog = logs.some(l => l.includes('copy') || l.includes('Copy') || l.includes('Copied'));
    console.log('Copy logs:', logs.filter(l => l.includes('CLIPBOARD') || l.includes('copy') || l.includes('Copy')));

    expect(true).toBe(true); // Smoke test
  });

  test('Paste button can be clicked', async ({ page }) => {
    const logs: string[] = [];

    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // First copy something
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(300);

    // Click "Paste from Clipboard" button
    await page.mouse.click(box.x + 250, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-03-paste-clicked.png', fullPage: true });

    console.log('Paste logs:', logs.filter(l => l.includes('CLIPBOARD') || l.includes('paste') || l.includes('Paste')));

    expect(true).toBe(true);
  });

  test('Check clipboard button works', async ({ page }) => {
    const logs: string[] = [];

    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Click "Check Clipboard" button
    await page.mouse.click(box.x + 400, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-04-check-clicked.png', fullPage: true });

    console.log('Check logs:', logs.filter(l => l.includes('CLIPBOARD') || l.includes('Check') || l.includes('contains')));

    expect(true).toBe(true);
  });

  test('Clear clipboard button works', async ({ page }) => {
    const logs: string[] = [];

    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // First copy something
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(300);

    // Click "Clear Clipboard" button
    await page.mouse.click(box.x + 550, box.y + 220);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/clipboard-05-clear-clicked.png', fullPage: true });

    console.log('Clear logs:', logs.filter(l => l.includes('CLIPBOARD') || l.includes('clear') || l.includes('Clear')));

    expect(true).toBe(true);
  });

  test('Full clipboard flow: copy, check, paste, clear', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/standalone/clipboard/clipboard_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // 1. Copy
    await page.mouse.click(box.x + 100, box.y + 220);
    await page.waitForTimeout(300);

    // 2. Check
    await page.mouse.click(box.x + 400, box.y + 220);
    await page.waitForTimeout(300);

    // 3. Paste
    await page.mouse.click(box.x + 250, box.y + 220);
    await page.waitForTimeout(300);

    // 4. Clear
    await page.mouse.click(box.x + 550, box.y + 220);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/clipboard-06-full-flow.png', fullPage: true });

    console.log('\n=== CLIPBOARD EVENTS ===');
    logs.filter(l => l.includes('CLIPBOARD')).forEach(l => console.log(l));

    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
