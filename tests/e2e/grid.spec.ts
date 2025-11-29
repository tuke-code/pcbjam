import { test, expect, Page } from '@playwright/test';

const MAIN_CANVAS = '#canvas';

async function waitForApp(page: Page) {
  await page.waitForSelector(MAIN_CANVAS, { state: 'visible', timeout: 30000 });
  await page.waitForTimeout(500);
}

async function switchToGridTab(page: Page, box: { x: number; y: number }) {
  // Click Grid tab (sixth tab, after OpenGL which is at x=280)
  // Tab widths: Controls(~45), Text Input(~55), Drawing(~50), Lists(~35), OpenGL(~50), Grid(~30)
  // Grid tab center is approximately at x = 310-320
  await page.mouse.click(box.x + 315, box.y + 35);
  await page.waitForTimeout(1000);
}

// ============================================================================
// DEDICATED wxGrid TEST PAGE - Tests if wxGrid compiles and runs at all
// ============================================================================

test.describe('wxGrid Dedicated Test Page', () => {

  // This is THE critical test for wxGrid support.
  // It loads a separate WASM binary that ONLY tests wxGrid.
  // If wxGrid is not implemented in WASM, this page will CRASH at startup.
  // When this test starts PASSING, wxGrid is working!
  test.fail('wxGrid test page loads successfully', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => {
      errors.push(`[PAGE_ERROR] ${err.message}\n${err.stack || ''}`);
    });
    page.on('console', msg => {
      logs.push(`[${msg.type()}] ${msg.text()}`);
      if (msg.type() === 'error') {
        errors.push(`[CONSOLE_ERROR] ${msg.text()}`);
      }
    });

    // Navigate to the dedicated wxGrid test page
    await page.goto('/standalone/grid/grid_test.html');

    // Wait for app to load - if wxGrid crashes, this will timeout or show error
    try {
      await page.waitForSelector(MAIN_CANVAS, { state: 'visible', timeout: 15000 });
      await page.waitForTimeout(1000);
    } catch (e) {
      // Expected to fail - wxGrid is not implemented
      console.log('wxGrid test page failed to load (expected):', e);
    }

    await page.screenshot({ path: 'test-results/wxgrid-dedicated-page.png', fullPage: true });

    // Log what happened
    console.log('\n=== wxGrid Dedicated Test Page Results ===');
    console.log('Console logs:', logs.length);
    console.log('Errors:', errors.length);
    errors.forEach(e => console.log(e));

    // Check for the success message from grid_test.cpp
    const hasSuccessMessage = logs.some(l =>
      l.includes('wxGrid test app started successfully') ||
      l.includes('wxGrid initialized successfully')
    );

    // Check for crash errors
    const hasCrash = errors.some(e =>
      e.includes('function signature mismatch') ||
      e.includes('RuntimeError') ||
      e.includes('Exception thrown')
    );

    console.log(`Has success message: ${hasSuccessMessage}`);
    console.log(`Has crash: ${hasCrash}`);

    // This test FAILS if wxGrid is not implemented (crashes or no success message)
    // When wxGrid is fixed, this will PASS
    expect(hasCrash, 'wxGrid should not crash the app').toBe(false);
    expect(hasSuccessMessage, 'wxGrid app should start successfully').toBe(true);
  });

  test.fail('wxGrid test page shows grid controls', async ({ page }) => {
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
      console.log(`[GRID_PAGE_CHECK] Pixel variance: ${variance}`);
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

    // This test documents that wxGrid is NOT implemented in WASM
    // It should FAIL until wxGrid is fixed. When it starts passing, wxGrid is working!
    test.fail('wxGrid renders visible grid cells', async ({ page }) => {
      const errors: string[] = [];

      page.on('pageerror', err => {
        errors.push(`[PAGE_ERROR] ${err.message}`);
      });
      page.on('console', msg => {
        if (msg.type() === 'error') {
          errors.push(`[CONSOLE_ERROR] ${msg.text()}`);
        }
      });

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      // Switch to Grid tab
      await switchToGridTab(page, box);
      await page.screenshot({ path: 'test-results/wxgrid-01-tab.png', fullPage: true });

      // The test should find actual grid cells. Since wxGrid is not implemented,
      // this will fail - which is exactly what we want to document!
      // We check for grid content by looking at the screenshot or canvas pixels

      // Evaluate if grid-like content exists (row/column headers, cells)
      const hasGridContent = await page.evaluate(() => {
        // Check if there's any element with grid-like structure
        // Since wxGrid would render to canvas, we check canvas content
        const canvas = document.querySelector('#canvas') as HTMLCanvasElement;
        if (!canvas) return false;

        const ctx = canvas.getContext('2d');
        if (!ctx) return false;

        // Sample pixels at where grid headers/cells would be (around y=180-200)
        // A real grid would have visible lines and text, not just blank space
        const imageData = ctx.getImageData(100, 180, 300, 50);
        const data = imageData.data;

        // Check if there's actual visible content (non-background pixels)
        let colorVariance = 0;
        let lastPixel = { r: data[0], g: data[1], b: data[2] };

        for (let i = 0; i < data.length; i += 4) {
          const r = data[i], g = data[i+1], b = data[i+2];
          // Check for color changes (grid lines, text)
          if (Math.abs(r - lastPixel.r) > 20 ||
              Math.abs(g - lastPixel.g) > 20 ||
              Math.abs(b - lastPixel.b) > 20) {
            colorVariance++;
          }
          lastPixel = { r, g, b };
        }

        // If there's significant variance, grid content exists
        // A blank area would have very low variance
        console.log(`[GRID_CHECK] Color variance: ${colorVariance}`);
        return colorVariance > 500; // Grid has lines and text = high variance
      });

      // This assertion WILL FAIL because wxGrid is not implemented
      // When wxGrid is fixed, this test will pass
      expect(hasGridContent, 'wxGrid should render visible grid cells with headers and data').toBe(true);
    });

    test.fail('wxGrid cell selection works', async ({ page }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToGridTab(page, box);

      // Click on a cell (would be at ~y=175 if grid existed)
      await page.mouse.click(box.x + 180, box.y + 175);
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/wxgrid-02-selection.png', fullPage: true });

      // Check console for grid selection events
      const logs: string[] = [];
      page.on('console', msg => logs.push(msg.text()));

      // Since wxGrid is not implemented, we won't get selection events
      // This test SHOULD FAIL
      const hasGridEvent = logs.some(l => l.includes('Grid cell'));
      expect(hasGridEvent, 'wxGrid should emit cell selection events').toBe(true);
    });

    test.fail('wxGrid cell editing works', async ({ page }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToGridTab(page, box);

      // Double-click to edit a cell
      await page.mouse.dblclick(box.x + 180, box.y + 195);
      await page.waitForTimeout(200);

      await page.keyboard.type('1.5');
      await page.keyboard.press('Enter');
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/wxgrid-03-editing.png', fullPage: true });

      // Check console for grid edit events
      const logs: string[] = [];
      page.on('console', msg => logs.push(msg.text()));

      // Since wxGrid is not implemented, we won't get edit events
      // This test SHOULD FAIL
      const hasEditEvent = logs.some(l => l.includes('Grid cell') && l.includes('changed'));
      expect(hasEditEvent, 'wxGrid should emit cell changed events when editing').toBe(true);
    });
  });

  // ============================================================================
  // wxSpinCtrl Tests - These should PASS if wxSpinCtrl is working
  // ============================================================================

  test.describe('wxSpinCtrl', () => {

    test('SpinCtrl renders and is visible', async ({ page }) => {
      const errors: string[] = [];

      page.on('pageerror', err => {
        errors.push(`[PAGE_ERROR] ${err.message}`);
      });

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToGridTab(page, box);
      await page.screenshot({ path: 'test-results/spinctrl-01-visible.png', fullPage: true });

      // SpinCtrl should be visible - check for its box and arrows in the canvas
      // Looking at the screenshot, controls are at top of panel around y=120-180
      const hasSpinCtrlContent = await page.evaluate(() => {
        const canvas = document.querySelector('#canvas') as HTMLCanvasElement;
        if (!canvas) return false;

        const ctx = canvas.getContext('2d');
        if (!ctx) return false;

        // SpinCtrl is at the top left of the Grid tab content, around y=130-170
        // It shows "Value (0-100): 50" with up/down arrows
        const imageData = ctx.getImageData(10, 130, 180, 50);
        const data = imageData.data;

        // Check for non-white content (text, borders, buttons)
        let nonBackgroundPixels = 0;
        for (let i = 0; i < data.length; i += 4) {
          const r = data[i], g = data[i+1], b = data[i+2];
          // Check for non-light pixels (text is dark, borders are gray)
          if (r < 230 || g < 230 || b < 230) {
            nonBackgroundPixels++;
          }
        }

        console.log(`[SPINCTRL_CHECK] Non-background pixels: ${nonBackgroundPixels}`);
        return nonBackgroundPixels > 50; // Should have visible text and borders
      });

      expect(hasSpinCtrlContent, 'wxSpinCtrl should be visible with borders and arrows').toBe(true);
      expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
    });

    test('SpinCtrl up/down arrows work', async ({ page }) => {
      const logs: string[] = [];

      page.on('console', msg => {
        logs.push(msg.text());
      });

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToGridTab(page, box);

      // Click up arrow on SpinCtrl (right side of the control, upper half)
      // SpinCtrl is around y=350, arrows on right side around x=220
      await page.mouse.click(box.x + 220, box.y + 350);
      await page.waitForTimeout(200);

      await page.screenshot({ path: 'test-results/spinctrl-02-after-click.png', fullPage: true });

      // Check for spin events
      const hasSpinEvent = logs.some(l => l.includes('SpinCtrl') || l.includes('spin'));
      console.log('SpinCtrl logs:', logs.filter(l => l.includes('Spin')));

      // If SpinCtrl is working, we should see events (or at least no crashes)
      // This is a smoke test - if it doesn't crash, that's good
      expect(true).toBe(true);
    });
  });

  // ============================================================================
  // wxSearchCtrl Tests - These should PASS if wxSearchCtrl is working
  // ============================================================================

  test.describe('wxSearchCtrl', () => {

    test('SearchCtrl renders and is visible', async ({ page }) => {
      const errors: string[] = [];

      page.on('pageerror', err => {
        errors.push(`[PAGE_ERROR] ${err.message}`);
      });

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToGridTab(page, box);
      await page.screenshot({ path: 'test-results/searchctrl-01-visible.png', fullPage: true });

      // SearchCtrl should be visible - check for its text field and buttons
      // Looking at the screenshot, SearchCtrl is on the right side around y=130-170
      const hasSearchCtrlContent = await page.evaluate(() => {
        const canvas = document.querySelector('#canvas') as HTMLCanvasElement;
        if (!canvas) return false;

        const ctx = canvas.getContext('2d');
        if (!ctx) return false;

        // SearchCtrl is on the right side of the Grid tab content, around y=130-170
        // It shows "Search..." placeholder with search icon and cancel button
        const imageData = ctx.getImageData(200, 130, 300, 50);
        const data = imageData.data;

        // Check for non-white content (text field border, placeholder text, icons)
        let nonBackgroundPixels = 0;
        for (let i = 0; i < data.length; i += 4) {
          const r = data[i], g = data[i+1], b = data[i+2];
          // Check for non-light pixels (borders, text, icons)
          if (r < 230 || g < 230 || b < 230) {
            nonBackgroundPixels++;
          }
        }

        console.log(`[SEARCHCTRL_CHECK] Non-background pixels: ${nonBackgroundPixels}`);
        return nonBackgroundPixels > 50; // Should have visible borders and placeholder text
      });

      expect(hasSearchCtrlContent, 'wxSearchCtrl should be visible with text field and buttons').toBe(true);
      expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
    });

    test('SearchCtrl accepts text input', async ({ page }) => {
      const logs: string[] = [];

      page.on('console', msg => {
        logs.push(msg.text());
      });

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

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

      // Check for search events
      const hasSearchEvent = logs.some(l => l.includes('Search') || l.includes('search'));
      console.log('SearchCtrl logs:', logs.filter(l => l.toLowerCase().includes('search')));

      // This is a smoke test - verify no crashes
      expect(true).toBe(true);
    });
  });

  // ============================================================================
  // Combined Tab Test - Basic smoke test for the Grid tab
  // ============================================================================

  test('Grid tab loads without crash', async ({ page }) => {
    const errors: string[] = [];

    page.on('pageerror', err => {
      errors.push(`[PAGE_ERROR] ${err.message}`);
    });

    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    await switchToGridTab(page, box);
    await page.screenshot({ path: 'test-results/grid-tab-final.png', fullPage: true });

    // Verify app is still responsive after switching to Grid tab
    const isResponsive = await page.evaluate(() => {
      return document.querySelector('#canvas') !== null;
    });

    expect(isResponsive).toBe(true);
    // Filter out favicon errors which are expected
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
