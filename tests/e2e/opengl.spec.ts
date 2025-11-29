import { test, expect, MAIN_CANVAS, waitForApp, getCanvasBox } from './utils/fixtures';

async function switchToOpenGLTab(page: any, box: { x: number; y: number }) {
  // Click OpenGL tab (fifth tab, around x=280)
  await page.mouse.click(box.x + 280, box.y + 35);
  await page.waitForTimeout(1000);
}

// Open the test dropdown and select by index (0-based)
async function selectGLTest(page: any, box: { x: number; y: number }, index: number) {
  // Click the dropdown arrow to open it
  await page.mouse.click(box.x + 290, box.y + 155);
  await page.waitForTimeout(300);

  // Each dropdown item is approximately 20px tall, starting below the dropdown
  const itemY = 175 + (index * 20);
  await page.mouse.click(box.x + 170, box.y + itemY);
  await page.waitForTimeout(500);
}

test.describe('OpenGL Tests', () => {
  test('Vertex Arrays test - debug freeze', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const box = await getCanvasBox(page);

    // Switch to OpenGL tab
    await switchToOpenGLTab(page, box);
    await page.screenshot({ path: 'test-results/gl-01-opengl-tab.png', fullPage: true });

    // Select "Vertex Arrays" test (index 2)
    await selectGLTest(page, box, 2);

    await page.screenshot({ path: 'test-results/gl-02-vertex-arrays-selected.png', fullPage: true });

    // Wait a bit to see if it freezes
    await page.waitForTimeout(2000);

    await page.screenshot({ path: 'test-results/gl-03-after-wait.png', fullPage: true });

    // Check if app is still responsive
    const isResponsive = await page.evaluate(() => {
      return document.querySelector('#canvas') !== null;
    });
    expect(isResponsive).toBe(true);
  });

  test('All GL tests individually', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const box = await getCanvasBox(page);

    await switchToOpenGLTab(page, box);

    const testNames = [
      'Immediate Mode',
      'Matrix Ops',
      'Vertex Arrays',
      'State Mgmt',
      'Texture Coords'
    ];

    for (let i = 0; i < testNames.length; i++) {
      try {
        await selectGLTest(page, box, i);
        await page.screenshot({
          path: `test-results/gl-test-${i}-${testNames[i].replace(/\s+/g, '-').toLowerCase()}.png`,
          fullPage: true
        });
      } catch (e) {
        await page.screenshot({
          path: `test-results/gl-test-${i}-${testNames[i].replace(/\s+/g, '-').toLowerCase()}-error.png`,
          fullPage: true
        });
      }

      await page.waitForTimeout(500);
    }
  });

  test('Run All Tests button', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    const box = await getCanvasBox(page);

    await switchToOpenGLTab(page, box);

    // Click "Run All Tests" button (approximately x=360, y=90)
    await page.mouse.click(box.x + 360, box.y + 90);
    await page.waitForTimeout(2000);

    await page.screenshot({ path: 'test-results/gl-run-all-tests.png', fullPage: true });

    // App should remain stable
    await expect(page.locator(MAIN_CANVAS)).toBeVisible();
  });
});
