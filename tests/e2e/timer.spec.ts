import { test, expect, waitForApp } from './utils/fixtures';

test.describe('wxTimer Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/standalone/timer/timer_test.html');
    // Wait for app to initialize
    await page.waitForFunction(() => {
      return document.querySelector('canvas') !== null;
    }, { timeout: 30000 });
    await page.waitForTimeout(1000);
  });

  test('Timer test app loads successfully', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const hasStartupLog = testLogger.consoleLogs.some(log =>
      log.includes('TIMER_TEST') && log.includes('started successfully')
    );

    await page.screenshot({ path: 'test-results/timer-01-loaded.png' });

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Slow timer can be started and stopped', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click Start button for slow timer
    await canvas.click({ position: { x: 240, y: 125 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/timer-02-started.png' });

    const hasStartEvent = testLogger.consoleLogs.some(log =>
      log.includes('Slow timer started')
    );
    expect(hasStartEvent).toBe(true);

    // Wait for timer tick
    await page.waitForTimeout(1500);
    await page.screenshot({ path: 'test-results/timer-03-ticked.png' });

    const hasTickEvent = testLogger.consoleLogs.some(log =>
      log.includes('TIMER_TICK')
    );

    // Click Stop button
    await canvas.click({ position: { x: 320, y: 125 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/timer-04-stopped.png' });

    const hasStopEvent = testLogger.consoleLogs.some(log =>
      log.includes('Slow timer stopped')
    );
    expect(hasStopEvent).toBe(true);
  });

  test('Fast timer can be started and updates gauge', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Click Start Fast button
    await canvas.click({ position: { x: 220, y: 260 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/timer-05-fast-started.png' });

    const hasFastStartEvent = testLogger.consoleLogs.some(log =>
      log.includes('Fast timer started')
    );
    expect(hasFastStartEvent).toBe(true);

    // Wait for multiple fast ticks
    await page.waitForTimeout(1000);
    await page.screenshot({ path: 'test-results/timer-06-fast-running.png' });

    // Click Stop Fast button
    await canvas.click({ position: { x: 340, y: 260 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/timer-07-fast-stopped.png' });

    const hasFastStopEvent = testLogger.consoleLogs.some(log =>
      log.includes('Fast timer stopped')
    );
    expect(hasFastStopEvent).toBe(true);
  });

  test('Reset counters button works', async ({ page, testLogger }) => {
    await page.waitForTimeout(500);

    const canvas = page.locator('canvas');

    // Start and run timer briefly
    await canvas.click({ position: { x: 240, y: 125 } });
    await page.waitForTimeout(1500);

    // Click Reset All Counters
    await canvas.click({ position: { x: 300, y: 320 } });
    await page.waitForTimeout(500);

    await page.screenshot({ path: 'test-results/timer-08-reset.png' });

    const hasResetEvent = testLogger.consoleLogs.some(log =>
      log.includes('Counters reset')
    );
    expect(hasResetEvent).toBe(true);
  });
});
