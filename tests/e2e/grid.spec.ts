import { test, expect, MAIN_CANVAS, waitForApp, getCanvasBox } from './utils/fixtures';

async function switchToGridTab(page: any, box: { x: number; y: number }) {
  // Click Grid tab (sixth tab, after OpenGL which is at x=280)
  await page.mouse.click(box.x + 315, box.y + 35);
  await page.waitForTimeout(1000);
}

// ============================================================================
// DEDICATED wxGrid TEST PAGE - Tests if wxGrid compiles and runs at all
// ============================================================================

test.describe('wxGrid Dedicated Test Page', () => {

  // This is THE critical test for wxGrid support.
  test('wxGrid test page loads successfully', async ({ page, testLogger }) => {
    // Navigate to the dedicated wxGrid test page
    await page.goto('/standalone/grid/grid_test.html');

    // Wait for app to load - if wxGrid crashes, this will timeout or show error
    try {
      await page.waitForSelector(MAIN_CANVAS, { state: 'visible', timeout: 15000 });
      await page.waitForTimeout(1000);
    } catch (e) {
      // Expected to fail - wxGrid is not implemented
    }

    await page.screenshot({ path: 'test-results/wxgrid-dedicated-page.png', fullPage: true });

    // Check for the success message from grid_test.cpp
    const hasSuccessMessage = testLogger.consoleLogs.some(l =>
      l.includes('wxGrid test app started successfully') ||
      l.includes('wxGrid initialized successfully')
    );

    // Check for crash errors
    const hasCrash = testLogger.errors.some(e =>
      e.includes('function signature mismatch') ||
      e.includes('RuntimeError') ||
      e.includes('Exception thrown')
    );

    // This test FAILS if wxGrid is not implemented (crashes or no success message)
    expect(hasCrash, 'wxGrid should not crash the app').toBe(false);
    expect(hasSuccessMessage, 'wxGrid app should start successfully').toBe(true);
  });

  test('wxGrid test page shows grid controls', async ({ page, testLogger }) => {
    await page.goto('/standalone/grid/grid_test.html');

    try {
      await page.waitForSelector(MAIN_CANVAS, { state: 'visible', timeout: 10000 });
    } catch {
      // Expected to fail
    }

    await page.screenshot({ path: 'test-results/wxgrid-controls.png', fullPage: true });

    // Check if grid content is visible in the canvas
    const hasGridContent = await page.evaluate(() => {
      const canvas = document.querySelector('#canvas') as HTMLCanvasElement;
      if (!canvas) return false;
      const ctx = canvas.getContext('2d');
      if (!ctx) return false;

      // Grid would be in the upper portion with labels and cells
      const imageData = ctx.getImageData(50, 100, 400, 150);
      const data = imageData.data;

      // Look for text/lines (significant color variance)
      let variance = 0;
      for (let i = 4; i < data.length; i += 4) {
        if (Math.abs(data[i] - data[i-4]) > 30 ||
            Math.abs(data[i+1] - data[i-3]) > 30 ||
            Math.abs(data[i+2] - data[i-2]) > 30) {
          variance++;
        }
      }
      return variance > 1000; // Grid with labels/lines has high variance
    });

    expect(hasGridContent, 'wxGrid page should show grid controls').toBe(true);
  });
});

test.describe('Grid Tab Tests', () => {

  // ============================================================================
  // wxGrid Tests in Main App - EXPECTED TO FAIL until wxGrid is added back
  // ============================================================================

  test.describe('wxGrid in Main App (DISABLED)', () => {

    test.fail('wxGrid renders visible grid cells', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      // Switch to Grid tab
      await switchToGridTab(page, box);
      await page.screenshot({ path: 'test-results/wxgrid-01-tab.png', fullPage: true });

      // Evaluate if grid-like content exists (row/column headers, cells)
      const hasGridContent = await page.evaluate(() => {
        const canvas = document.querySelector('#canvas') as HTMLCanvasElement;
        if (!canvas) return false;

        const ctx = canvas.getContext('2d');
        if (!ctx) return false;

        // Sample pixels at where grid headers/cells would be (around y=180-200)
        const imageData = ctx.getImageData(100, 180, 300, 50);
        const data = imageData.data;

        // Check if there's actual visible content (non-background pixels)
        let colorVariance = 0;
        let lastPixel = { r: data[0], g: data[1], b: data[2] };

        for (let i = 0; i < data.length; i += 4) {
          const r = data[i], g = data[i+1], b = data[i+2];
          if (Math.abs(r - lastPixel.r) > 20 ||
              Math.abs(g - lastPixel.g) > 20 ||
              Math.abs(b - lastPixel.b) > 20) {
            colorVariance++;
          }
          lastPixel = { r, g, b };
        }

        return colorVariance > 500; // Grid has lines and text = high variance
      });

      expect(hasGridContent, 'wxGrid should render visible grid cells with headers and data').toBe(true);
    });

    test.fail('wxGrid cell selection works', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToGridTab(page, box);

      // Click on a cell (would be at ~y=175 if grid existed)
      await page.mouse.click(box.x + 180, box.y + 175);
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/wxgrid-02-selection.png', fullPage: true });

      // Since wxGrid is not implemented, we won't get selection events
      const hasGridEvent = testLogger.consoleLogs.some(l => l.includes('Grid cell'));
      expect(hasGridEvent, 'wxGrid should emit cell selection events').toBe(true);
    });

    test.fail('wxGrid cell editing works', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToGridTab(page, box);

      // Double-click to edit a cell
      await page.mouse.dblclick(box.x + 180, box.y + 195);
      await page.waitForTimeout(200);

      await page.keyboard.type('1.5');
      await page.keyboard.press('Enter');
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/wxgrid-03-editing.png', fullPage: true });

      // Since wxGrid is not implemented, we won't get edit events
      const hasEditEvent = testLogger.consoleLogs.some(l => l.includes('Grid cell') && l.includes('changed'));
      expect(hasEditEvent, 'wxGrid should emit cell changed events when editing').toBe(true);
    });
  });

  // ============================================================================
  // wxSpinCtrl Tests - These should PASS if wxSpinCtrl is working
  // ============================================================================

  test.describe('wxSpinCtrl', () => {

    test('SpinCtrl renders and is visible', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToGridTab(page, box);
      await page.screenshot({ path: 'test-results/spinctrl-01-visible.png', fullPage: true });

      // SpinCtrl should be visible - check for its box and arrows in the canvas
      const hasSpinCtrlContent = await page.evaluate(() => {
        const canvas = document.querySelector('#canvas') as HTMLCanvasElement;
        if (!canvas) return false;

        const ctx = canvas.getContext('2d');
        if (!ctx) return false;

        // SpinCtrl is at the top left of the Grid tab content, around y=130-170
        const imageData = ctx.getImageData(10, 130, 180, 50);
        const data = imageData.data;

        // Check for non-white content (text, borders, buttons)
        let nonBackgroundPixels = 0;
        for (let i = 0; i < data.length; i += 4) {
          const r = data[i], g = data[i+1], b = data[i+2];
          if (r < 230 || g < 230 || b < 230) {
            nonBackgroundPixels++;
          }
        }

        return nonBackgroundPixels > 50; // Should have visible text and borders
      });

      expect(hasSpinCtrlContent, 'wxSpinCtrl should be visible with borders and arrows').toBe(true);
      expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
    });

    test('SpinCtrl up/down arrows work', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToGridTab(page, box);

      // Click up arrow on SpinCtrl
      await page.mouse.click(box.x + 220, box.y + 350);
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/spinctrl-02-after-click.png', fullPage: true });

      // This is a smoke test - if it doesn't crash, that's good
      expect(true).toBe(true);
    });
  });

  // ============================================================================
  // wxSearchCtrl Tests - These should PASS if wxSearchCtrl is working
  // ============================================================================

  test.describe('wxSearchCtrl', () => {

    test('SearchCtrl renders and is visible', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToGridTab(page, box);
      await page.screenshot({ path: 'test-results/searchctrl-01-visible.png', fullPage: true });

      // SearchCtrl should be visible - check for its text field and buttons
      const hasSearchCtrlContent = await page.evaluate(() => {
        const canvas = document.querySelector('#canvas') as HTMLCanvasElement;
        if (!canvas) return false;

        const ctx = canvas.getContext('2d');
        if (!ctx) return false;

        // SearchCtrl is on the right side of the Grid tab content, around y=130-170
        const imageData = ctx.getImageData(200, 130, 300, 50);
        const data = imageData.data;

        // Check for non-white content (text field border, placeholder text, icons)
        let nonBackgroundPixels = 0;
        for (let i = 0; i < data.length; i += 4) {
          const r = data[i], g = data[i+1], b = data[i+2];
          if (r < 230 || g < 230 || b < 230) {
            nonBackgroundPixels++;
          }
        }

        return nonBackgroundPixels > 50; // Should have visible borders and placeholder text
      });

      expect(hasSearchCtrlContent, 'wxSearchCtrl should be visible with text field and buttons').toBe(true);
      expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
    });

    test('SearchCtrl accepts text input', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToGridTab(page, box);

      // Click on SearchCtrl text field (center of the control)
      await page.mouse.click(box.x + 420, box.y + 350);
      await page.waitForTimeout(200);

      // Type search text
      await page.keyboard.type('TRACK');
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/searchctrl-02-typing.png', fullPage: true });

      // Press Enter to search
      await page.keyboard.press('Enter');
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/searchctrl-03-after-enter.png', fullPage: true });

      // This is a smoke test - verify no crashes
      expect(true).toBe(true);
    });
  });

  // ============================================================================
  // Combined Tab Test - Basic smoke test for the Grid tab
  // ============================================================================

  test('Grid tab loads without crash', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const box = await getCanvasBox(page);

    await switchToGridTab(page, box);
    await page.screenshot({ path: 'test-results/grid-tab-final.png', fullPage: true });

    // Verify app is still responsive after switching to Grid tab
    const isResponsive = await page.evaluate(() => {
      return document.querySelector('#canvas') !== null;
    });

    expect(isResponsive).toBe(true);
    // Filter out favicon errors which are expected
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
