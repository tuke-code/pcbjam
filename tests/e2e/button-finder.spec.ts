/**
 * Button Finder Utility
 *
 * A utility for finding clickable button positions in wxWidgets WASM canvas apps.
 * This is NOT a regular test - it's excluded from normal test runs via testIgnore.
 *
 * wxWidgets WASM renders everything to a canvas, so UI tests need specific pixel
 * coordinates to click buttons. This utility scans the canvas and reports positions
 * that trigger console log responses (indicating a button was clicked).
 *
 * Usage:
 *   cd tests
 *
 *   # Scan clipboard test app
 *   APP_URL=/standalone/clipboard/clipboard_test.html npx playwright test button-finder --reporter=list
 *
 *   # Scan dialog test app
 *   APP_URL=/standalone/dialog/dialog_test.html npx playwright test button-finder --reporter=list
 *
 *   # Scan tree test app
 *   APP_URL=/standalone/tree/tree_test.html npx playwright test button-finder --reporter=list
 *
 *   # Scan menu test app
 *   APP_URL=/standalone/menu/menu_test.html npx playwright test button-finder --reporter=list
 *
 *   # Scan with custom region (faster - focus on likely button area)
 *   APP_URL=/standalone/dialog/dialog_test.html START_Y=150 END_Y=300 STEP=8 npx playwright test button-finder --reporter=list
 *
 * Environment Variables:
 *   APP_URL   - URL path to scan (REQUIRED - no default to force explicit choice)
 *   STEP      - Pixel step size (default: 10, smaller = more accurate but slower)
 *   START_X   - X start coordinate (default: 0)
 *   END_X     - X end coordinate (default: canvas width)
 *   START_Y   - Y start coordinate (default: 0)
 *   END_Y     - Y end coordinate (default: canvas height)
 *
 * Available Test Apps:
 *   /standalone/clipboard/clipboard_test.html  - Copy, Paste, Check, Clear buttons
 *   /standalone/dialog/dialog_test.html        - Info, Yes/No, Error, Custom dialog buttons
 *   /standalone/tree/tree_test.html            - Expand All, Collapse All, etc.
 *   /standalone/menu/menu_test.html            - Menu bar testing
 *   /standalone/grid/grid_test.html            - Grid controls
 *   /standalone/aui/aui_test.html              - AUI panel controls
 *   /standalone/toolbar/toolbar_test.html      - Toolbar buttons
 *   /standalone/timer/timer_test.html          - Timer controls
 *   /standalone/filedialog/filedialog_test.html - File dialog buttons
 *   /standalone/layout/layout_test.html        - Layout controls
 *
 * Output:
 *   - Console output with button positions and labels
 *   - Generated test code snippets
 *   - JSON results at test-results/button-finder-results.json
 */

import { test, expect } from '@playwright/test';
import { tryLoadApp } from './utils/fixtures';

// Configuration from environment - APP_URL is required
const APP_URL = process.env.APP_URL;
const STEP = parseInt(process.env.STEP || '10');
const START_Y = process.env.START_Y ? parseInt(process.env.START_Y) : undefined;
const END_Y = process.env.END_Y ? parseInt(process.env.END_Y) : undefined;
const START_X = process.env.START_X ? parseInt(process.env.START_X) : undefined;
const END_X = process.env.END_X ? parseInt(process.env.END_X) : undefined;

test.describe('Button Finder Utility', () => {
  // Long timeout for scanning
  test.setTimeout(300000);

  test('Scan for buttons', async ({ page }) => {
    // Require APP_URL to be specified
    if (!APP_URL) {
      console.error('\n' + '='.repeat(70));
      console.error('ERROR: APP_URL environment variable is required');
      console.error('='.repeat(70));
      console.error('\nUsage examples:');
      console.error('  APP_URL=/standalone/clipboard/clipboard_test.html npx playwright test button-finder --reporter=list');
      console.error('  APP_URL=/standalone/dialog/dialog_test.html npx playwright test button-finder --reporter=list');
      console.error('  APP_URL=/standalone/tree/tree_test.html START_Y=100 END_Y=300 npx playwright test button-finder --reporter=list');
      console.error('\nAvailable apps:');
      console.error('  /standalone/clipboard/clipboard_test.html');
      console.error('  /standalone/dialog/dialog_test.html');
      console.error('  /standalone/tree/tree_test.html');
      console.error('  /standalone/menu/menu_test.html');
      console.error('  /standalone/grid/grid_test.html');
      console.error('  /standalone/aui/aui_test.html');
      console.error('  /standalone/toolbar/toolbar_test.html');
      console.error('  /standalone/timer/timer_test.html');
      console.error('  /standalone/filedialog/filedialog_test.html');
      console.error('  /standalone/layout/layout_test.html');
      console.error('');
      test.skip();
      return;
    }

    console.log(`\n${'='.repeat(70)}`);
    console.log(`Button Finder - Scanning: ${APP_URL}`);
    console.log(`Step size: ${STEP}px`);
    console.log(`${'='.repeat(70)}\n`);

    // Navigate and wait for app
    await page.goto(APP_URL);
    const loaded = await tryLoadApp(page);
    expect(loaded, 'App should load').toBe(true);

    // Get canvas bounds
    const canvas = page.locator('canvas').first();
    const box = await canvas.boundingBox();
    if (!box) {
      console.log('ERROR: Canvas not found');
      return;
    }

    const scanStartX = START_X ?? 0;
    const scanEndX = END_X ?? box.width;
    const scanStartY = START_Y ?? 0;
    const scanEndY = END_Y ?? box.height;

    console.log(`Canvas: ${box.width}x${box.height} at (${box.x}, ${box.y})`);
    console.log(`Scan region: X[${scanStartX}-${scanEndX}] Y[${scanStartY}-${scanEndY}]`);
    console.log(`Estimated clicks: ${Math.ceil((scanEndX - scanStartX) / STEP) * Math.ceil((scanEndY - scanStartY) / STEP)}\n`);

    // Take before screenshot
    await page.screenshot({ path: 'test-results/button-finder-before.png', fullPage: true });

    // Collect console logs
    const logs: string[] = [];
    page.on('console', msg => logs.push(msg.text()));

    // Track found buttons
    const buttons: Array<{x: number, y: number, label: string, log: string}> = [];
    const seenLogs = new Set<string>();

    // Keywords that indicate a button click response
    const buttonKeywords = [
      'Attempting', 'SUCCESS', 'ERROR', 'WARNING',
      'clicked', 'Clicked', 'EVT_BUTTON', 'EVT_MENU',
      'OnButton', 'OnClick', 'pressed', 'Pressed',
      'Copy', 'Paste', 'Clear', 'Check', 'Open', 'Close',
      'selected', 'Selected', 'expand', 'collapse',
      'Expand', 'Collapse', 'Add', 'Remove', 'Delete',
      'Start', 'Stop', 'Reset', 'Save', 'Load'
    ];

    // Label keywords for identification
    const labelKeywords = [
      'Copy', 'Paste', 'Clear', 'Check', 'Open', 'Close',
      'OK', 'Cancel', 'Yes', 'No', 'Expand', 'Collapse',
      'Add', 'Remove', 'Delete', 'Start', 'Stop', 'Reset',
      'Save', 'Load', 'Info', 'Error', 'Warning', 'Custom'
    ];

    // Scan the canvas
    let lastProgressY = -100;
    for (let y = scanStartY; y < scanEndY; y += STEP) {
      // Progress indicator every 50px
      if (y - lastProgressY >= 50) {
        console.log(`Scanning row ${y}/${scanEndY}...`);
        lastProgressY = y;
      }

      for (let x = scanStartX; x < scanEndX; x += STEP) {
        const logCountBefore = logs.length;

        // Click at this position
        await page.mouse.click(box.x + x, box.y + y);
        // Small wait to allow response
        await page.waitForTimeout(20);

        // Check for new logs
        if (logs.length > logCountBefore) {
          const newLogs = logs.slice(logCountBefore);

          for (const log of newLogs) {
            // Skip noise
            if (log.includes('favicon') || log.includes('DevTools')) continue;

            // Check if this looks like a button response
            const isButtonLog = buttonKeywords.some(kw => log.includes(kw));

            if (isButtonLog && !seenLogs.has(log)) {
              seenLogs.add(log);

              // Try to extract a label
              let label = 'Button';
              for (const kw of labelKeywords) {
                if (log.toLowerCase().includes(kw.toLowerCase())) {
                  label = kw;
                  break;
                }
              }

              buttons.push({
                x,
                y,
                label,
                log: log.substring(0, 80)
              });

              console.log(`  FOUND: (${x}, ${y}) ${label} - "${log.substring(0, 60)}"`);
            }
          }
        }
      }
    }

    // Take after screenshot
    await page.screenshot({ path: 'test-results/button-finder-after.png', fullPage: true });

    // Output results
    console.log(`\n${'='.repeat(70)}`);
    console.log(`RESULTS: Found ${buttons.length} buttons`);
    console.log(`${'='.repeat(70)}\n`);

    if (buttons.length > 0) {
      console.log('Button positions (relative to canvas):');
      console.log('');
      for (const btn of buttons) {
        console.log(`  ${btn.label.padEnd(12)} at (${String(btn.x).padStart(3)}, ${String(btn.y).padStart(3)})`);
        console.log(`    Log: ${btn.log}`);
        console.log('');
      }

      console.log('\nGenerated test code:');
      console.log('```typescript');
      console.log('const box = await getCanvasBox(page);');
      console.log('');
      for (const btn of buttons) {
        console.log(`// ${btn.label} button`);
        console.log(`await page.mouse.click(box.x + ${btn.x}, box.y + ${btn.y});`);
        console.log(`await page.waitForTimeout(500);`);
        console.log('');
      }
      console.log('```');
    } else {
      console.log('No buttons found.');
      console.log('\nPossible reasons:');
      console.log('  - Buttons are outside the scan region (try adjusting START_Y/END_Y)');
      console.log('  - Button clicks dont produce recognizable logs');
      console.log('  - Step size is too large (try STEP=5)');
      console.log('\nCaptured logs:');
      logs.slice(0, 30).forEach(log => console.log(`  ${log.substring(0, 80)}`));
    }

    // Write results to a JSON file for programmatic use
    const results = {
      app: APP_URL,
      canvas: { x: box.x, y: box.y, width: box.width, height: box.height },
      scanRegion: { startX: scanStartX, endX: scanEndX, startY: scanStartY, endY: scanEndY },
      step: STEP,
      buttons: buttons
    };

    const fs = await import('fs');
    fs.writeFileSync('test-results/button-finder-results.json', JSON.stringify(results, null, 2));
    console.log('\nResults saved to: test-results/button-finder-results.json');
  });
});
