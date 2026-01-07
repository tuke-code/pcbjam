/**
 * GAL WebGL Regression Test
 *
 * Runs all 28 GAL test scenarios in WebGL and captures screenshots
 * for comparison against native OpenGL rendering.
 */

import { test, expect } from './utils/fixtures';
import * as path from 'path';
import * as fs from 'fs';

// Scenario names (must match native test)
const SCENARIO_NAMES = [
  'basic-lines',      // 0
  'line-widths',      // 1
  'circles',          // 2
  'arcs',             // 3
  'rectangles',       // 4
  'polygons',         // 5
  'alpha-blending',   // 6
  'transforms',       // 7
  'grid-cursor',      // 8
  'segments',         // 9
  'complex-scene',    // 10
  'bezier-curves',    // 11
  'arc-segments',     // 12
  'segment-chain',    // 13
  'group-caching',    // 14
  'polylines-multi',  // 15
  'hole-walls',       // 16
  'grid-native',      // 17
  'cursor-native',    // 18
  'render-targets',   // 19
  'screen-transform', // 20
  'clear-colors',     // 21
  'depth-testing',    // 22
  'negative-mode',    // 23
  'text-attrs',       // 24
  'glyphs',           // 25
  'bitmap',           // 26
  'transform-api'     // 27
];

// Output directory for WebGL screenshots
const OUTPUT_DIR = path.join(__dirname, '../gal-regression/output/webgl');

test.describe('GAL WebGL Regression Tests', () => {
  test.beforeAll(async () => {
    // Ensure output directory exists
    if (!fs.existsSync(OUTPUT_DIR)) {
      fs.mkdirSync(OUTPUT_DIR, { recursive: true });
    }
  });

  test('Load GAL WebGL test module', async ({ page, testLogger }) => {
    await page.goto('/gal-webgl/gal_webgl_test.html');

    // Wait for the custom event indicating module is ready
    await page.waitForFunction(() => {
      return (window as any).galTest !== undefined;
    }, { timeout: 60000 });

    // Verify module loaded
    const totalScenarios = await page.evaluate(() => {
      return (window as any).galTest.getTotalScenarios();
    });

    expect(totalScenarios).toBe(28);

    await page.screenshot({
      path: path.join(OUTPUT_DIR, 'gal-module-loaded.png'),
      fullPage: true
    });

    console.log(`GAL WebGL test module loaded with ${totalScenarios} scenarios`);
  });

  // Generate a test for each scenario
  for (let i = 0; i < SCENARIO_NAMES.length; i++) {
    const scenarioName = SCENARIO_NAMES[i];
    const scenarioIndex = i;

    test(`Scenario ${scenarioIndex}: ${scenarioName}`, async ({ page, testLogger }) => {
      await page.goto('/gal-webgl/gal_webgl_test.html');

      // Wait for module to be ready
      await page.waitForFunction(() => {
        return (window as any).galTest !== undefined;
      }, { timeout: 60000 });

      // Run the scenario
      await page.evaluate((index) => {
        (window as any).galTest.runScenario(index);
      }, scenarioIndex);

      // Wait for rendering to complete
      await page.waitForTimeout(100);

      // Get the canvas element and take a screenshot of just the canvas
      const canvas = await page.locator('#canvas');
      await expect(canvas).toBeVisible();

      // Screenshot the canvas (matching native 800x600 output)
      const screenshotPath = path.join(OUTPUT_DIR, `gal-${scenarioName}.png`);
      await canvas.screenshot({ path: screenshotPath });

      console.log(`Saved: ${screenshotPath}`);
    });
  }

  test('Run all scenarios sequentially', async ({ page, testLogger }) => {
    await page.goto('/gal-webgl/gal_webgl_test.html');

    // Wait for module to be ready
    await page.waitForFunction(() => {
      return (window as any).galTest !== undefined;
    }, { timeout: 60000 });

    console.log('Running all 28 scenarios...');

    for (let i = 0; i < SCENARIO_NAMES.length; i++) {
      const scenarioName = SCENARIO_NAMES[i];

      // Run scenario
      await page.evaluate((index) => {
        (window as any).galTest.runScenario(index);
      }, i);

      // Wait for rendering
      await page.waitForTimeout(50);

      // Screenshot the canvas
      const canvas = await page.locator('#canvas');
      const screenshotPath = path.join(OUTPUT_DIR, `gal-${scenarioName}.png`);
      await canvas.screenshot({ path: screenshotPath });

      console.log(`[${i + 1}/28] ${scenarioName}`);
    }

    console.log('All scenarios completed');
  });
});
