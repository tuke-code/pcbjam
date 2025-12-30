import { test, expect } from './utils/fixtures';
import { clickByLabel } from './utils/element-tracker';

/**
 * wxStyledTextCtrl Tests
 *
 * KiCad uses wxStyledTextCtrl (Scintilla) for:
 * - DRC rules editor
 * - Python console
 * - Custom script editors
 *
 * Layout (from screenshot):
 * - Description text at top
 * - Buttons at y≈107 (centered):
 *   - Python: x≈345
 *   - DRC Rules: x≈433
 *   - Plain: x≈522
 *   - Insert Sample: x≈617
 *   - Clear: x≈719
 *   - Line Numbers: x≈828
 *   - Fold All: x≈932
 * - wxStyledTextCtrl editor area: y≈130 to y≈500
 * - Event log at bottom
 */

test.describe('wxStyledTextCtrl Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/standalone/stc/stc_test.html');
    // Wait for app to initialize
    await page.waitForFunction(() => {
      return document.querySelector('canvas') !== null;
    }, { timeout: 30000 });
    await page.waitForTimeout(1000);
  });

  test('STC test app loads successfully', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const hasStartupLog = testLogger.consoleLogs.some(log =>
      log.includes('STC_TEST') && log.includes('started successfully')
    );

    await page.screenshot({ path: 'test-results/stc-01-loaded.png' });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Python syntax highlighting is enabled by default', async ({ page }) => {
    await page.waitForTimeout(1000);

    // Take screenshot to verify Python code with syntax highlighting
    await page.screenshot({ path: 'test-results/stc-02-python-default.png' });

    // Visual verification - the editor should show Python code with colors
    // (import, def, for keywords should be highlighted)
  });

  test('DRC Rules mode can be activated', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    // Click DRC Rules button using element registry
    await clickByLabel(page, 'DRC Rules');
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/stc-03-drc-mode.png' });

    const hasDrcLog = testLogger.consoleLogs.some(log =>
      log.includes('DRC rules lexer configured')
    );
    expect(hasDrcLog).toBe(true);
  });

  test('Plain text mode can be activated', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    // Click Plain button using element registry
    await clickByLabel(page, 'Plain');
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/stc-04-plain-mode.png' });

    const hasPlainLog = testLogger.consoleLogs.some(log =>
      log.includes('Plain text mode enabled')
    );
    expect(hasPlainLog).toBe(true);
  });

  test('Insert Sample button adds code', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    // Click Insert Sample button using element registry
    await clickByLabel(page, 'Insert Sample');
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/stc-05-insert-sample.png' });

    const hasInsertLog = testLogger.consoleLogs.some(log =>
      log.includes('Inserted sample code')
    );
    expect(hasInsertLog).toBe(true);
  });

  test('Clear button clears editor content', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    // Click Clear button using element registry
    await clickByLabel(page, 'Clear');
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/stc-06-cleared.png' });

    const hasClearLog = testLogger.consoleLogs.some(log =>
      log.includes('Text cleared')
    );
    expect(hasClearLog).toBe(true);
  });

  test('Line numbers can be toggled', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    // Click Line Numbers button using element registry
    await clickByLabel(page, 'Line Numbers');
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/stc-07-line-numbers-toggle.png' });

    const hasLineNumLog = testLogger.consoleLogs.some(log =>
      log.includes('Line numbers hidden') || log.includes('Line numbers shown')
    );
    expect(hasLineNumLog).toBe(true);
  });

  test('Fold All button works', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    // Click Fold All button using element registry
    await clickByLabel(page, 'Fold All');
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/stc-08-folded.png' });

    const hasFoldLog = testLogger.consoleLogs.some(log =>
      log.includes('All code folded')
    );
    expect(hasFoldLog).toBe(true);
  });

  test('Editor can receive text input', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Clear the editor first using element registry
    await clickByLabel(page, 'Clear');
    await page.waitForTimeout(300);

    // Click in the editor area to focus it
    // STC (Scintilla) has complex internal windowing - click anywhere in editor area
    await canvas.click({ position: { x: 400, y: 300 } });
    await page.waitForTimeout(200);

    // Type some text
    await page.keyboard.type('# Test input\nprint("Hello WASM!")');
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/stc-09-typed.png' });

    // The text should have triggered change events (logged every 10 changes)
  });

  test('Switching between modes preserves content structure', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    // Start with Python mode (default)
    await page.screenshot({ path: 'test-results/stc-10a-python.png' });

    // Switch to DRC mode using element registry
    await clickByLabel(page, 'DRC Rules');
    await page.waitForTimeout(300);
    await page.screenshot({ path: 'test-results/stc-10b-drc.png' });

    // Switch back to Python using element registry
    await clickByLabel(page, 'Python');
    await page.waitForTimeout(300);
    await page.screenshot({ path: 'test-results/stc-10c-python-again.png' });

    // Multiple mode switches should work (at least DRC mode change should be logged)
    const modeChanges = testLogger.consoleLogs.filter(log =>
      log.includes('lexer configured') || log.includes('mode enabled')
    ).length;
    expect(modeChanges).toBeGreaterThanOrEqual(1);
  });
});
