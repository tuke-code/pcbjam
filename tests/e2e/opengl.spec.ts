import { test, expect, MAIN_CANVAS, waitForApp, getCanvasBox } from './utils/fixtures';
import { clickTab, clickByLabel, findByLabel, clickListItem } from './utils/element-tracker';

async function switchToOpenGLTab(page: any) {
  // Click OpenGL tab using element registry
  const clicked = await clickTab(page, 'OpenGL');
  if (!clicked) {
    // Fallback: try clicking by label
    await clickByLabel(page, 'OpenGL');
  }
  await page.waitForTimeout(1000);
}

// Open the test dropdown and select by name
async function selectGLTest(page: any, testName: string) {
  // Click the dropdown (it's a choice control with label "Select Test:")
  const dropdownClicked = await clickByLabel(page, 'Select Test:');
  if (!dropdownClicked) {
    // Fallback: try finding by partial label
    await clickByLabel(page, 'Immediate Mode'); // Click currently selected value
  }
  await page.waitForTimeout(300);

  // Click the test item in the dropdown list
  const itemClicked = await clickListItem(page, testName);
  if (!itemClicked) {
    // Fallback: try by label
    await clickByLabel(page, testName);
  }
  await page.waitForTimeout(500);
}

test.describe('OpenGL Tests', () => {
  test('Vertex Arrays test - debug freeze', async ({ page, testLogger }) => {
    await page.goto('/minimal_test.html');
    await waitForApp(page);

    // Switch to OpenGL tab using element registry
    await switchToOpenGLTab(page);
    await page.screenshot({ path: 'test-results/gl-01-opengl-tab.png', fullPage: true });

    // Select "Vertex Arrays" test by name
    await selectGLTest(page, 'Vertex Arrays');

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

    await switchToOpenGLTab(page);

    const testNames = [
      'Immediate Mode',
      'Matrix Ops',
      'Vertex Arrays',
      'State Mgmt',
      'Texture Coords'
    ];

    for (let i = 0; i < testNames.length; i++) {
      try {
        await selectGLTest(page, testNames[i]);
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

    await switchToOpenGLTab(page);

    // Click "Run All Tests" button using element registry
    const clicked = await clickByLabel(page, 'Run All Tests');
    expect(clicked, 'Run All Tests button should be found and clicked').toBe(true);
    await page.waitForTimeout(2000);

    await page.screenshot({ path: 'test-results/gl-run-all-tests.png', fullPage: true });

    // App should remain stable
    await expect(page.locator(MAIN_CANVAS)).toBeVisible();
  });
});
