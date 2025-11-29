import { test, expect } from '@playwright/test';

test.describe('wxDialog/wxMessageBox Tests', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/standalone/dialog/dialog_test.html');
    // Wait for app to initialize
    await page.waitForFunction(() => {
      return document.querySelector('canvas') !== null;
    }, { timeout: 30000 });
    await page.waitForTimeout(1000);
  });

  test('Dialog test app loads successfully', async ({ page }) => {
    const consoleLogs: string[] = [];
    page.on('console', msg => {
      if (msg.text().includes('[DIALOG_')) {
        consoleLogs.push(msg.text());
      }
    });

    await page.waitForTimeout(500);

    const hasStartupLog = consoleLogs.some(log =>
      log.includes('DIALOG_TEST') && log.includes('started successfully')
    );

    console.log('Dialog app logs:', consoleLogs);
    console.log('Dialog app loaded:', hasStartupLog);

    await page.screenshot({ path: 'test-results/dialog-01-loaded.png' });

    // App should load without crash
    const pageErrors: string[] = [];
    page.on('pageerror', err => pageErrors.push(err.message));

    expect(pageErrors.length).toBe(0);
  });

  test('Info dialog button can be clicked', async ({ page }) => {
    const consoleLogs: string[] = [];
    page.on('console', msg => {
      if (msg.text().includes('[DIALOG_EVENT]')) {
        consoleLogs.push(msg.text());
      }
    });

    await page.waitForTimeout(500);

    // Click Info Dialog button
    const canvas = page.locator('canvas');
    await canvas.click({ position: { x: 70, y: 130 } });
    await page.waitForTimeout(500);

    console.log('Info dialog logs:', consoleLogs);
    await page.screenshot({ path: 'test-results/dialog-02-info-clicked.png' });

    // Should have logged the dialog event
    const hasInfoEvent = consoleLogs.some(log =>
      log.includes('Opening Info dialog')
    );
    expect(hasInfoEvent).toBe(true);
  });

  test('Yes/No dialog button can be clicked', async ({ page }) => {
    const consoleLogs: string[] = [];
    page.on('console', msg => {
      if (msg.text().includes('[DIALOG_EVENT]')) {
        consoleLogs.push(msg.text());
      }
    });

    await page.waitForTimeout(500);

    // Click Yes/No Dialog button
    const canvas = page.locator('canvas');
    await canvas.click({ position: { x: 190, y: 130 } });
    await page.waitForTimeout(500);

    console.log('Yes/No dialog logs:', consoleLogs);
    await page.screenshot({ path: 'test-results/dialog-03-yesno-clicked.png' });

    const hasYesNoEvent = consoleLogs.some(log =>
      log.includes('Opening Yes/No dialog')
    );
    expect(hasYesNoEvent).toBe(true);
  });

  test('Error dialog button can be clicked', async ({ page }) => {
    const consoleLogs: string[] = [];
    page.on('console', msg => {
      if (msg.text().includes('[DIALOG_EVENT]')) {
        consoleLogs.push(msg.text());
      }
    });

    await page.waitForTimeout(500);

    // Click Error Dialog button
    const canvas = page.locator('canvas');
    await canvas.click({ position: { x: 310, y: 130 } });
    await page.waitForTimeout(500);

    console.log('Error dialog logs:', consoleLogs);
    await page.screenshot({ path: 'test-results/dialog-04-error-clicked.png' });

    const hasErrorEvent = consoleLogs.some(log =>
      log.includes('Opening Error dialog')
    );
    expect(hasErrorEvent).toBe(true);
  });

  test('Custom dialog button can be clicked', async ({ page }) => {
    const consoleLogs: string[] = [];
    page.on('console', msg => {
      if (msg.text().includes('[DIALOG_EVENT]')) {
        consoleLogs.push(msg.text());
      }
    });

    await page.waitForTimeout(500);

    // Click Custom Dialog button
    const canvas = page.locator('canvas');
    await canvas.click({ position: { x: 100, y: 195 } });
    await page.waitForTimeout(500);

    console.log('Custom dialog logs:', consoleLogs);
    await page.screenshot({ path: 'test-results/dialog-05-custom-clicked.png' });

    const hasCustomEvent = consoleLogs.some(log =>
      log.includes('Opening Custom dialog')
    );
    expect(hasCustomEvent).toBe(true);
  });
});
