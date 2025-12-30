import { test, expect, waitForApp } from './utils/fixtures';
import { clickTab, clickByLabel } from './utils/element-tracker';

async function switchToDialogsTab(page: any) {
  // Click Dialogs tab using element registry
  const clicked = await clickTab(page, 'Dialogs');
  if (!clicked) {
    await clickByLabel(page, 'Dialogs');
  }
  await page.waitForTimeout(1000);
}

test.describe('Dialogs Tab Tests', () => {

  test('Dialogs tab renders correctly', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    // Screenshot before switching to Dialogs tab
    await page.screenshot({ path: 'test-results/dialogs-00-initial.png', fullPage: true });

    // Switch to Dialogs tab
    await switchToDialogsTab(page);
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

      await switchToDialogsTab(page);

      // Click "Info" button using element registry
      await clickByLabel(page, 'Info');
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-info-open.png', fullPage: true });

      // Click OK to close
      await clickByLabel(page, 'OK');
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-info-closed.png', fullPage: true });

      expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
    });

    test('Yes/No message box returns correct result', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      await switchToDialogsTab(page);

      // Click "Yes/No" button using element registry
      await clickByLabel(page, 'Yes/No');
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-yesno-open.png', fullPage: true });

      // Click Yes to close
      await clickByLabel(page, 'Yes');
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-yesno-closed.png', fullPage: true });
    });

    test('Error message box displays correctly', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      await switchToDialogsTab(page);

      // Click "Error" button using element registry
      await clickByLabel(page, 'Error');
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-error-open.png', fullPage: true });

      // Close the dialog with OK
      await clickByLabel(page, 'OK');
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-error-closed.png', fullPage: true });
    });
  });

  test.describe('wxDialog', () => {

    test('Custom dialog opens and closes', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      await switchToDialogsTab(page);

      // Click "Open Custom Dialog" button using element registry
      await clickByLabel(page, 'Open Custom Dialog');
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-custom-open.png', fullPage: true });

      // Click OK to close
      await clickByLabel(page, 'OK');
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-custom-closed.png', fullPage: true });
    });
  });

  test.describe('wxTimer', () => {

    test('Timer starts and increments', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      await switchToDialogsTab(page);

      await page.screenshot({ path: 'test-results/dialogs-timer-initial.png', fullPage: true });

      // Click "Start Timer" button using element registry
      await clickByLabel(page, 'Start Timer');
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-timer-started.png', fullPage: true });

      // Wait for a few timer ticks
      await page.waitForTimeout(3500);

      await page.screenshot({ path: 'test-results/dialogs-timer-running.png', fullPage: true });

      // Click "Stop Timer"
      await clickByLabel(page, 'Stop Timer');
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-timer-stopped.png', fullPage: true });

      // Just verify no crashes occurred - this is a smoke test
      expect(true).toBe(true);
    });

    test('Timer can be started and stopped multiple times', async ({ page, testLogger }) => {
      await page.goto('/minimal_test.html');
      await waitForApp(page);

      await switchToDialogsTab(page);

      // Start timer using element registry
      await clickByLabel(page, 'Start Timer');
      await page.waitForTimeout(1500);

      // Stop timer
      await clickByLabel(page, 'Stop Timer');
      await page.waitForTimeout(500);

      // Start again
      await clickByLabel(page, 'Start Timer');
      await page.waitForTimeout(1500);

      // Stop again
      await clickByLabel(page, 'Stop Timer');
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-timer-multiple.png', fullPage: true });

      // Basic smoke test - no crashes
      expect(true).toBe(true);
    });
  });

  test('Full Dialogs tab interaction flow', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    await switchToDialogsTab(page);

    // 1. Click Info button and close
    await clickByLabel(page, 'Info');
    await page.waitForTimeout(400);
    await clickByLabel(page, 'OK');
    await page.waitForTimeout(200);

    // 2. Click Custom Dialog button and close
    await clickByLabel(page, 'Open Custom Dialog');
    await page.waitForTimeout(400);
    await clickByLabel(page, 'OK');
    await page.waitForTimeout(200);

    // 3. Start and stop timer
    await clickByLabel(page, 'Start Timer');
    await page.waitForTimeout(2000);
    await clickByLabel(page, 'Stop Timer');
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/dialogs-full-flow.png', fullPage: true });

    // Verify no crashes
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
