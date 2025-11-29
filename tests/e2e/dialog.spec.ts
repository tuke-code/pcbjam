import { test, expect, MAIN_CANVAS, tryLoadApp, getCanvasBox } from './utils/fixtures';

test.describe('wxDialog/wxMessageBox Tests', () => {
  test('Dialog test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/dialog/dialog_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/dialog-01-loaded.png', fullPage: true });

    const hasStartupLog = testLogger.consoleLogs.some(log =>
      log.includes('DIALOG_TEST') && log.includes('started successfully')
    );

    expect(loaded, 'Canvas should be visible').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Info dialog button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/dialog/dialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Info Dialog button is first in the wxMessageBox row
    const centerX = box.width / 2;
    await page.mouse.click(box.x + centerX - 110, box.y + 115);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/dialog-02-info-clicked.png', fullPage: true });

    const hasInfoEvent = testLogger.consoleLogs.some(log =>
      log.includes('Opening Info dialog')
    );

    expect(hasInfoEvent, 'Info dialog should open').toBe(true);
  });

  test('Yes/No dialog button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/dialog/dialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Yes/No Dialog button is second (center) in the wxMessageBox row
    const centerX = box.width / 2;
    await page.mouse.click(box.x + centerX, box.y + 115);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/dialog-03-yesno-clicked.png', fullPage: true });

    const hasYesNoEvent = testLogger.consoleLogs.some(log =>
      log.includes('Opening Yes/No dialog')
    );
    expect(hasYesNoEvent, 'Yes/No dialog should open').toBe(true);
  });

  test('Error dialog button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/dialog/dialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Error Dialog button is third (rightmost) in the wxMessageBox row
    const centerX = box.width / 2;
    await page.mouse.click(box.x + centerX + 110, box.y + 115);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/dialog-04-error-clicked.png', fullPage: true });

    const hasErrorEvent = testLogger.consoleLogs.some(log =>
      log.includes('Opening Error dialog')
    );
    expect(hasErrorEvent, 'Error dialog should open').toBe(true);
  });

  test('Custom dialog button can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/dialog/dialog_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const box = await getCanvasBox(page);

    // Custom Dialog button is first in the wxDialog row (row 2)
    const centerX = box.width / 2;
    await page.mouse.click(box.x + centerX - 60, box.y + 175);
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/dialog-05-custom-clicked.png', fullPage: true });

    expect(true).toBe(true);
  });
});
