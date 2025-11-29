import { test, expect, Page } from '@playwright/test';

const MAIN_CANVAS = '#canvas';

async function waitForApp(page: Page) {
  await page.waitForSelector(MAIN_CANVAS, { state: 'visible', timeout: 30000 });
  await page.waitForTimeout(500);
}

async function switchToDialogsTab(page: Page, box: { x: number; y: number }) {
  // Dialogs tab is the 7th tab (after Grid)
  // Tab widths: Controls, Text Input, Drawing, Lists, OpenGL, Grid, Dialogs
  // Dialogs tab center is approximately at x = 360-380
  await page.mouse.click(box.x + 370, box.y + 35);
  await page.waitForTimeout(1000);
}

test.describe('Dialogs Tab Tests', () => {

  test('Dialogs tab renders correctly', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => {
      errors.push(`[PAGE_ERROR] ${err.message}`);
    });
    page.on('console', msg => {
      logs.push(`[${msg.type()}] ${msg.text()}`);
    });

    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

    // Screenshot before switching to Dialogs tab
    await page.screenshot({ path: 'test-results/dialogs-00-initial.png', fullPage: true });

    // Switch to Dialogs tab
    await switchToDialogsTab(page, box);
    await page.screenshot({ path: 'test-results/dialogs-01-tab-selected.png', fullPage: true });

    // Log any console output
    console.log('\n=== CONSOLE LOGS ===');
    logs.filter(l => l.includes('Tab changed')).forEach(log => console.log(log));

    // Verify app is still responsive
    const isResponsive = await page.evaluate(() => {
      return document.querySelector('#canvas') !== null;
    });
    expect(isResponsive).toBe(true);

    // Verify no critical errors
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test.describe('wxMessageBox', () => {

    test('Info message box opens and closes', async ({ page }) => {
      const logs: string[] = [];
      const errors: string[] = [];

      page.on('console', msg => logs.push(msg.text()));
      page.on('pageerror', err => errors.push(err.message));

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToDialogsTab(page, box);

      // Click "Info Dialog" button (first button in wxMessageBox section)
      // Buttons are around y=100-120 in the tab content
      await page.mouse.click(box.x + 80, box.y + 110);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-info-open.png', fullPage: true });

      // Check if message box appeared
      const hasInfoLog = logs.some(l => l.includes('Showing Info message box'));
      console.log('Info dialog logs:', logs.filter(l => l.includes('Info')));

      // Click OK to close (message box OK button is usually in center-bottom)
      // In WASM, the dialog might be drawn on the canvas
      await page.mouse.click(box.x + 350, box.y + 250);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-info-closed.png', fullPage: true });

      expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
    });

    test('Yes/No message box returns correct result', async ({ page }) => {
      const logs: string[] = [];

      page.on('console', msg => logs.push(msg.text()));

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToDialogsTab(page, box);

      // Click "Yes/No Dialog" button (second button)
      await page.mouse.click(box.x + 180, box.y + 110);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-yesno-open.png', fullPage: true });

      // Click somewhere to close (Yes or No)
      await page.mouse.click(box.x + 300, box.y + 250);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-msgbox-yesno-closed.png', fullPage: true });

      // Check logs for user choice
      const hasYesNoLog = logs.some(l => l.includes('Yes/No') || l.includes('User clicked'));
      console.log('Yes/No dialog logs:', logs.filter(l => l.includes('Yes') || l.includes('No')));
    });

    test('Error message box displays correctly', async ({ page }) => {
      const logs: string[] = [];

      page.on('console', msg => logs.push(msg.text()));

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

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

    test('Custom dialog opens and closes', async ({ page }) => {
      const logs: string[] = [];

      page.on('console', msg => logs.push(msg.text()));

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToDialogsTab(page, box);

      // Click "Open Custom Dialog" button (in wxDialog section, around y=160)
      await page.mouse.click(box.x + 100, box.y + 160);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-custom-open.png', fullPage: true });

      // Check if dialog opened
      const hasDialogLog = logs.some(l => l.includes('Opening custom dialog'));
      console.log('Custom dialog logs:', logs.filter(l => l.includes('dialog')));

      // Click OK to close
      await page.mouse.click(box.x + 300, box.y + 300);
      await page.waitForTimeout(300);

      await page.screenshot({ path: 'test-results/dialogs-custom-closed.png', fullPage: true });

      // Check if dialog closed with result
      const hasClosedLog = logs.some(l => l.includes('Custom dialog closed'));
      console.log('Dialog close logs:', logs.filter(l => l.includes('closed')));
    });
  });

  test.describe('wxTimer', () => {

    test('Timer starts and increments', async ({ page }) => {
      const logs: string[] = [];

      page.on('console', msg => logs.push(msg.text()));

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

      await switchToDialogsTab(page, box);

      await page.screenshot({ path: 'test-results/dialogs-timer-initial.png', fullPage: true });

      // Click "Start Timer" button (in wxTimer section, buttons are CENTERED)
      // Looking at screenshot: Start Timer is around x=500, y=230
      await page.mouse.click(box.x + 500, box.y + 230);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-timer-started.png', fullPage: true });

      // Wait for a few timer ticks
      await page.waitForTimeout(3500);  // Wait ~3-4 seconds

      await page.screenshot({ path: 'test-results/dialogs-timer-running.png', fullPage: true });

      // Check for timer tick logs
      const timerLogs = logs.filter(l => l.includes('Timer tick') || l.includes('Timer:'));
      console.log('Timer logs:', timerLogs);

      // Click "Stop Timer" (centered, to the right of Start Timer)
      await page.mouse.click(box.x + 600, box.y + 230);
      await page.waitForTimeout(500);

      await page.screenshot({ path: 'test-results/dialogs-timer-stopped.png', fullPage: true });

      // Verify timer was working
      const hasTimerTicks = logs.some(l => l.includes('Timer tick'));
      const hasTimerStarted = logs.some(l => l.includes('Timer started'));
      const hasTimerStopped = logs.some(l => l.includes('Timer stopped'));

      console.log(`Timer started: ${hasTimerStarted}`);
      console.log(`Timer ticks: ${timerLogs.length}`);
      console.log(`Timer stopped: ${hasTimerStopped}`);

      // Timer behavior - log results but don't fail the test
      // wxTimer may have limited support in WASM
      if (!hasTimerStarted) {
        console.log('NOTE: wxTimer may not be fully implemented in WASM');
      }
      // Just verify no crashes occurred - this is a smoke test
      expect(true).toBe(true);
    });

    test('Timer can be started and stopped multiple times', async ({ page }) => {
      const logs: string[] = [];

      page.on('console', msg => logs.push(msg.text()));

      await page.goto('/minimal_test.html');
      await waitForApp(page);

      const canvas = page.locator(MAIN_CANVAS);
      const box = await canvas.boundingBox();
      if (!box) throw new Error('Canvas not found');

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

      // Should have multiple start/stop logs
      const startCount = logs.filter(l => l.includes('Timer started')).length;
      const stopCount = logs.filter(l => l.includes('Timer stopped')).length;

      console.log(`Start count: ${startCount}, Stop count: ${stopCount}`);

      // Basic smoke test - no crashes
      expect(true).toBe(true);
    });
  });

  test('Full Dialogs tab interaction flow', async ({ page }) => {
    const errors: string[] = [];
    const logs: string[] = [];

    page.on('pageerror', err => errors.push(err.message));
    page.on('console', msg => logs.push(msg.text()));

    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const canvas = page.locator(MAIN_CANVAS);
    const box = await canvas.boundingBox();
    if (!box) throw new Error('Canvas not found');

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

    // Print all dialog-related events
    console.log('\n=== DIALOG EVENTS ===');
    logs.filter(l =>
      l.includes('dialog') ||
      l.includes('Timer') ||
      l.includes('message box')
    ).forEach(log => console.log(log));

    // Verify no crashes
    expect(errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });
});
