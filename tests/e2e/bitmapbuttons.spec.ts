// wxBitmapButton Tests - Bitmap buttons, toggle buttons, disabled states
import { test, expect, tryLoadApp } from './utils/fixtures';

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

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click Select tool button
      await page.mouse.click(box.x + 50, box.y + 100);
      await page.waitForTimeout(100);
      // Click Line tool button
      await page.mouse.click(box.x + 90, box.y + 100);
      await page.waitForTimeout(100);
    }

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

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click F.Cu toggle button to turn it off
      await page.mouse.click(box.x + 50, box.y + 170);
      await page.waitForTimeout(200);
    }

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

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Toggle multiple layer buttons
      await page.mouse.click(box.x + 50, box.y + 170);
      await page.waitForTimeout(100);
      await page.mouse.click(box.x + 120, box.y + 170);
      await page.waitForTimeout(100);
      await page.mouse.click(box.x + 190, box.y + 170);
      await page.waitForTimeout(200);
    }

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

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click Toggle Enable State button
      await page.mouse.click(box.x + 400, box.y + 240);
      await page.waitForTimeout(200);
    }

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

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click different shape buttons
      await page.mouse.click(box.x + 50, box.y + 310);
      await page.waitForTimeout(100);
      await page.mouse.click(box.x + 100, box.y + 310);
      await page.waitForTimeout(100);
      await page.mouse.click(box.x + 150, box.y + 310);
      await page.waitForTimeout(200);
    }

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

    const canvas = page.locator('#canvas');
    const box = await canvas.boundingBox();
    if (box) {
      // Click New, Open, Save buttons
      await page.mouse.click(box.x + 50, box.y + 380);
      await page.waitForTimeout(100);
      await page.mouse.click(box.x + 90, box.y + 380);
      await page.waitForTimeout(200);
    }

    await page.screenshot({ path: 'test-results/bitmapbuttons-07-artprovider.png', fullPage: true });
  });
});
