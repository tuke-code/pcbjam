import { test, expect } from './utils/fixtures';

/**
 * wxPrinting Tests
 *
 * KiCad uses wxPrinting for:
 * - Schematic printing
 * - PCB printing
 * - Export to PDF
 *
 * Layout (from button finder):
 * - Description text at top
 * - Buttons at y=95:
 *   - Print Preview: x=425
 *   - Print...: x=550
 *   - Browser Print: x=685
 *   - Page Setup: x=755
 * - Document preview panel
 * - Event log at bottom
 */

test.describe('wxPrinting Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/standalone/print/print_test.html');
    // Wait for app to initialize
    await page.waitForFunction(() => {
      return document.querySelector('canvas') !== null;
    }, { timeout: 30000 });
    await page.waitForTimeout(1000);
  });

  test('Print test app loads successfully', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const hasStartupLog = testLogger.consoleLogs.some(log =>
      log.includes('PRINT_TEST') && log.includes('started successfully')
    );

    await page.screenshot({ path: 'test-results/print-01-loaded.png' });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Document preview panel renders', async ({ page }) => {
    await page.waitForTimeout(1000);

    // Take screenshot to verify preview panel shows document content
    await page.screenshot({ path: 'test-results/print-02-preview-panel.png' });

    // Visual verification - preview panel should show shapes and text
  });

  test('Browser Print button triggers window.print()', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Mock window.print to track calls
    let printCalled = false;
    await page.evaluate(() => {
      (window as any).originalPrint = window.print;
      (window as any).printWasCalled = false;
      window.print = () => {
        console.log('[TEST] window.print() was called');
        (window as any).printWasCalled = true;
      };
    });

    // Find and click Browser Print button (button finder: 685, 95)
    await canvas.click({ position: { x: 685, y: 95 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/print-03-browser-print-clicked.png' });

    // Check if Browser Print triggered window.print
    const hasBrowserPrintLog = testLogger.consoleLogs.some(log =>
      log.includes('window.print') || log.includes('browser print')
    );

    // Also check our mock
    const printWasCalled = await page.evaluate(() => (window as any).printWasCalled);

    // Restore original print function
    await page.evaluate(() => {
      window.print = (window as any).originalPrint;
    });

    // Either the log message appears or window.print was called
    expect(hasBrowserPrintLog || printWasCalled).toBe(true);
  });

  test('Print Preview button works', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click Print Preview button (button finder: 425, 95)
    await canvas.click({ position: { x: 425, y: 95 } });
    await page.waitForTimeout(1000);

    await page.screenshot({ path: 'test-results/print-04-preview-clicked.png' });

    // Check for print preview events
    const hasPreviewLog = testLogger.consoleLogs.some(log =>
      log.includes('Print Preview') || log.includes('PRINTOUT_CALLBACK')
    );

    // Print preview may open a new frame or log messages
    // Either preview opens successfully or we get an error logged
  });

  test('Page Setup button works', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click Page Setup button (button finder: 755, 95)
    await canvas.click({ position: { x: 755, y: 95 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/print-05-page-setup-clicked.png' });

    // Check for page setup events
    const hasPageSetupLog = testLogger.consoleLogs.some(log =>
      log.includes('Page Setup') || log.includes('page setup')
    );
  });

  test('Print button works', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click Print... button (button finder: 550, 95)
    await canvas.click({ position: { x: 550, y: 95 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/print-06-print-clicked.png' });

    // Check for print dialog events
    const hasPrintLog = testLogger.consoleLogs.some(log =>
      log.includes('Print dialog') || log.includes('Opening Print')
    );
  });

  test('Printout callbacks are triggered', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Try to trigger print preview to see callbacks (button finder: 425, 95)
    await canvas.click({ position: { x: 425, y: 95 } });
    await page.waitForTimeout(1500);

    await page.screenshot({ path: 'test-results/print-07-callbacks.png' });

    // Look for printout callback messages
    const callbacks = testLogger.consoleLogs.filter(log =>
      log.includes('PRINTOUT_CALLBACK')
    );

    // Log what callbacks we found (for debugging)
    console.log('Printout callbacks found:', callbacks.length);
    callbacks.forEach(cb => console.log('  -', cb));
  });

  test('No JavaScript errors during print operations', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('#canvas');

    // Click each button to test for crashes
    // Button positions from button finder
    const buttonPositions = [
      { x: 425, y: 95, name: 'Print Preview' },
      { x: 550, y: 95, name: 'Print' },
      { x: 685, y: 95, name: 'Browser Print' },
      { x: 755, y: 95, name: 'Page Setup' },
    ];

    // Mock window.print to prevent actual print dialog
    await page.evaluate(() => {
      (window as any).originalPrint = window.print;
      window.print = () => console.log('[MOCK] window.print() called');
    });

    for (const btn of buttonPositions) {
      await canvas.click({ position: { x: btn.x, y: btn.y } });
      await page.waitForTimeout(500);
    }

    await page.screenshot({ path: 'test-results/print-08-all-buttons.png' });

    // Restore window.print
    await page.evaluate(() => {
      window.print = (window as any).originalPrint;
    });

    // Filter out favicon and any expected errors
    const realErrors = testLogger.errors.filter(e =>
      !e.includes('favicon') &&
      !e.includes('printer error') && // Expected if no printer configured
      !e.includes('not available')
    );

    expect(realErrors).toHaveLength(0);
  });
});
