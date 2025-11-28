import { test, expect } from '@playwright/test';

test.describe('wxWidgets WASM', () => {
  test('app loads without JavaScript errors', async ({ page }) => {
    const errors: string[] = [];
    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => {
      if (msg.type() === 'error') {
        errors.push(msg.text());
      }
    });

    await page.goto('/minimal_test.html');

    // Wait for WASM to initialize (canvas becomes visible)
    await page.waitForSelector('canvas', { state: 'visible', timeout: 30000 });

    // Give it a moment to settle
    await page.waitForTimeout(1000);

    // Filter out known non-critical errors
    const criticalErrors = errors.filter(e =>
      !e.includes('SharedArrayBuffer') && // COOP/COEP warning
      !e.includes('cross-origin')
    );

    expect(criticalErrors).toHaveLength(0);
  });

  test('canvas element is rendered with dimensions', async ({ page }) => {
    await page.goto('/minimal_test.html');

    const canvas = page.locator('canvas');
    await expect(canvas).toBeVisible({ timeout: 30000 });

    // Canvas should have non-zero dimensions
    const box = await canvas.boundingBox();
    expect(box).not.toBeNull();
    expect(box?.width).toBeGreaterThan(0);
    expect(box?.height).toBeGreaterThan(0);
  });

  test('loading progress completes', async ({ page }) => {
    await page.goto('/minimal_test.html');

    // Progress text should eventually show completion or disappear
    await page.waitForFunction(() => {
      const status = document.getElementById('progress-text');
      if (!status) return true;
      const text = status.textContent || '';
      return text.toLowerCase().includes('complete') ||
             text === '' ||
             status.style.display === 'none';
    }, { timeout: 30000 });
  });

  test('WASM module initializes successfully', async ({ page }) => {
    await page.goto('/minimal_test.html');

    // Wait for canvas to be visible (indicates WASM loaded)
    await page.waitForSelector('canvas', { state: 'visible', timeout: 30000 });

    // Check that the Module object exists and is initialized
    const moduleExists = await page.evaluate(() => {
      return typeof (window as any).Module !== 'undefined';
    });

    expect(moduleExists).toBe(true);
  });
});
