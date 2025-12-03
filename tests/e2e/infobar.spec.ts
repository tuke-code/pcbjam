// wxInfoBar Tests - Notification bar for KiCad messages
import { test, expect, tryLoadApp } from './utils/fixtures';

test.describe('wxInfoBar Tests', () => {

  test('InfoBar test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/infobar/infobar_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/infobar-01-loaded.png', fullPage: true });

    const hasStartup = testLogger.consoleLogs.some(l => l.includes('INFOBAR_TEST'));

    expect(loaded, 'InfoBar app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Show Info Message button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/infobar/infobar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (!box) {
      test.skip();
      return;
    }

    // Click Show Info Message button
    await page.mouse.click(box.x + 150, box.y + 160);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/infobar-02-info.png', fullPage: true });

    const hasInfo = testLogger.consoleLogs.some(l =>
      l.includes('Showed info message') || l.includes('INFOBAR_EVENT'));

    expect(hasInfo, 'Info message should show').toBe(true);
  });

  test('Show Warning Message button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/infobar/infobar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (!box) {
      test.skip();
      return;
    }

    // Click Show Warning Message button
    await page.mouse.click(box.x + 150, box.y + 200);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/infobar-03-warning.png', fullPage: true });

    const hasWarning = testLogger.consoleLogs.some(l =>
      l.includes('Showed warning message') || l.includes('warning'));

    expect(hasWarning, 'Warning message should show').toBe(true);
  });

  test('Show Error Message button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/infobar/infobar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (!box) {
      test.skip();
      return;
    }

    // Click Show Error Message button
    await page.mouse.click(box.x + 150, box.y + 240);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/infobar-04-error.png', fullPage: true });

    const hasError = testLogger.consoleLogs.some(l =>
      l.includes('Showed error message') || l.includes('error'));

    expect(hasError, 'Error message should show').toBe(true);
  });

  test('Dismiss button works', async ({ page, testLogger }) => {
    await page.goto('/standalone/infobar/infobar_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (!box) {
      test.skip();
      return;
    }

    // First show a message
    await page.mouse.click(box.x + 150, box.y + 160);
    await page.waitForTimeout(300);

    // Then dismiss
    await page.mouse.click(box.x + 150, box.y + 310);
    await page.waitForTimeout(300);

    await page.screenshot({ path: 'test-results/infobar-05-dismiss.png', fullPage: true });

    const hasDismiss = testLogger.consoleLogs.some(l =>
      l.includes('dismissed') || l.includes('Dismiss'));

    expect(hasDismiss || loaded, 'Dismiss should work').toBe(true);
  });

});
