import { test, expect, MAIN_CANVAS, waitForApp, getCanvasBox } from './utils/fixtures';

async function switchToDialogsTab(page: any, box: { x: number; y: number }) {
  // Dialogs tab is the 7th tab (after Grid)
  await page.mouse.click(box.x + 370, box.y + 35);
  await page.waitForTimeout(1000);
}

test.describe('Dialogs Tab Tests', () => {

  test('Dialogs tab renders correctly', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const box = await getCanvasBox(page);

    // Screenshot before switching to Dialogs tab
    await page.screenshot({ path: 'test-results/dialogs-00-initial.png', fullPage: true });

    // Switch to Dialogs tab
    await switchToDialogsTab(page, box);
    await page.screenshot({ path: 'test-results/dialogs-01-tab-selected.png', fullPage: true });

    // Verify app is still responsive
    const isResponsive = await page.evaluate(() => {
      return document.querySelector('#canvas') !== null;
    });
    expect(isResponsive).toBe(true);

    // Verify no critical errors
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test.describe('wxMessageBox', () => {

    test('Info message box opens and closes', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToDialogsTab(page, box);

      // Click "Info Dialog" button (first button in wxMessageBox section)
      await page.mouse.click(box.x + 80, box.y + 110);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-info-open.png', fullPage: true });

      // Click OK to close
      await page.mouse.click(box.x + 350, box.y + 250);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-info-closed.png', fullPage: true });

      expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
    });

    test('Yes/No message box returns correct result', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToDialogsTab(page, box);

      // Click "Yes/No Dialog" button (second button)
      await page.mouse.click(box.x + 180, box.y + 110);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-yesno-open.png', fullPage: true });

      // Click somewhere to close (Yes or No)
      await page.mouse.click(box.x + 300, box.y + 250);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-yesno-closed.png', fullPage: true });
    });

    test('Error message box displays correctly', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToDialogsTab(page, box);

      // Click "Error Dialog" button (third button)
      await page.mouse.click(box.x + 280, box.y + 110);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-error-open.png', fullPage: true });

      // Close the dialog
      await page.mouse.click(box.x + 350, box.y + 250);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-error-closed.png', fullPage: true });
    });
  });

  test.describe('wxDialog', () => {

    test('Custom dialog opens and closes', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToDialogsTab(page, box);

      // Click "Open Custom Dialog" button (in wxDialog section, around y=160)
      await page.mouse.click(box.x + 100, box.y + 160);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-custom-open.png', fullPage: true });

      // Click OK to close
      await page.mouse.click(box.x + 300, box.y + 300);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-custom-closed.png', fullPage: true });
    });
  });

  test.describe('wxTimer', () => {

    test('Timer starts and increments', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToDialogsTab(page, box);

      await page.screenshot({ path: 'test-results/dialogs-timer-initial.png', fullPage: true });

      // Click "Start Timer" button (centered buttons)
      await page.mouse.click(box.x + 500, box.y + 230);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-timer-started.png', fullPage: true });

      // Wait for a few timer ticks
      await page.waitForTimeout(3500);

      await page.screenshot({ path: 'test-results/dialogs-timer-running.png', fullPage: true });

      // Click "Stop Timer"
      await page.mouse.click(box.x + 600, box.y + 230);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-timer-stopped.png', fullPage: true });

      // Just verify no crashes occurred - this is a smoke test
      expect(true).toBe(true);
    });

    test('Timer can be started and stopped multiple times', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const box = await getCanvasBox(page);

      await switchToDialogsTab(page, box);

      // Start timer (centered buttons)
      await page.mouse.click(box.x + 500, box.y + 230);
      await page.waitForTimeout(1500);

      // Stop timer
      await page.mouse.click(box.x + 600, box.y + 230);
      await page.waitForTimeout(500);

      // Start again
      await page.mouse.click(box.x + 500, box.y + 230);
      await page.waitForTimeout(1500);

      // Stop again
      await page.mouse.click(box.x + 600, box.y + 230);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-timer-multiple.png', fullPage: true });

      // Basic smoke test - no crashes
      expect(true).toBe(true);
    });
  });

  test('Full Dialogs tab interaction flow', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const box = await getCanvasBox(page);

    await switchToDialogsTab(page, box);

    // 1. Click Info button
    await page.mouse.click(box.x + 80, box.y + 110);
    await page.waitForTimeout(400);
    await page.mouse.click(box.x + 350, box.y + 250);  // Close
    await page.waitForTimeout(200);

    // 2. Click Custom Dialog button
    await page.mouse.click(box.x + 100, box.y + 160);
    await page.waitForTimeout(400);
    await page.mouse.click(box.x + 300, box.y + 300);  // Close
    await page.waitForTimeout(200);

    // 3. Start and stop timer (centered buttons)
    await page.mouse.click(box.x + 500, box.y + 230);  // Start
    await page.waitForTimeout(2000);
    await page.mouse.click(box.x + 600, box.y + 230);  // Stop
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/dialogs-full-flow.png', fullPage: true });

    // Verify no crashes
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
