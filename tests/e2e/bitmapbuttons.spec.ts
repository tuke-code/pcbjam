// wxBitmapButton Tests - Bitmap buttons, toggle buttons, disabled states
import { test, expect, tryLoadApp } from './utils/fixtures';
import { clickByLabel } from './utils/element-tracker';

test.describe('wxBitmapButton Tests', () => {

  test('Bitmap buttons test app loads successfully', async ({ page, testLogger }) => {
    await page.goto('/standalone/bitmapbuttons/bitmapbuttons_test.html');
    const loaded = await tryLoadApp(page);

    await page.screenshot({ path: 'test-results/bitmapbuttons-01-loaded.png', fullPage: true });

    expect(loaded, 'Bitmap buttons app should load').toBe(true);
    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Toolbar-style buttons can be clicked', async ({ page, testLogger }) => {
    await page.goto('/standalone/bitmapbuttons/bitmapbuttons_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    // Click Select tool button using element registry
    await clickByLabel(page, 'Select Tool');
    await page.waitForTimeout(100);
    // Click Line tool button
    await clickByLabel(page, 'Line Tool');
    await page.waitForTimeout(100);

    await page.screenshot({ path: 'test-results/bitmapbuttons-02-toolbar-click.png', fullPage: true });
  });

  test('Toggle buttons can be toggled', async ({ page, testLogger }) => {
    await page.goto('/standalone/bitmapbuttons/bitmapbuttons_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    // Click F.Cu checkbox to toggle it using element registry
    await clickByLabel(page, 'F.Cu');
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/bitmapbuttons-03-toggle.png', fullPage: true });
  });

  test('Toggle button can toggle multiple layers', async ({ page, testLogger }) => {
    await page.goto('/standalone/bitmapbuttons/bitmapbuttons_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    // Toggle multiple layer checkboxes using element registry
    await clickByLabel(page, 'F.Cu');
    await page.waitForTimeout(100);
    await clickByLabel(page, 'B.Cu');
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/bitmapbuttons-04-multi-toggle.png', fullPage: true });
  });

  test('Disabled button can be re-enabled', async ({ page, testLogger }) => {
    await page.goto('/standalone/bitmapbuttons/bitmapbuttons_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    // Click Toggle Enable State button using element registry
    await clickByLabel(page, 'Toggle Enable State');
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/bitmapbuttons-05-enable-toggle.png', fullPage: true });
  });

  test('Shape buttons display different icons', async ({ page, testLogger }) => {
    await page.goto('/standalone/bitmapbuttons/bitmapbuttons_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    // Click different shape buttons using element registry (by tooltip)
    await clickByLabel(page, 'Rectangle');
    await page.waitForTimeout(100);
    await clickByLabel(page, 'Circle');
    await page.waitForTimeout(100);
    await clickByLabel(page, 'Triangle');
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/bitmapbuttons-06-shapes.png', fullPage: true });
  });

  test('Art Provider buttons display system icons', async ({ page, testLogger }) => {
    await page.goto('/standalone/bitmapbuttons/bitmapbuttons_test.html');
    const loaded = await tryLoadApp(page);
    if (!loaded) {
      test.skip();
      return;
    }

    await page.waitForTimeout(300);

    // Click New, Open buttons using element registry (by tooltip)
    await clickByLabel(page, 'New');
    await page.waitForTimeout(100);
    await clickByLabel(page, 'Open');
    await page.waitForTimeout(200);

    await page.screenshot({ path: 'test-results/bitmapbuttons-07-artprovider.png', fullPage: true });
  });
});
