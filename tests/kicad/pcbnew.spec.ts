import { test, expect } from './fixtures';
import { clickByLabel, dumpElements } from '../e2e/utils/element-tracker';

/**
 * PCBnew WASM E2E Tests
 */

test.describe('PCBnew WASM', () => {
    test.beforeEach(async ({ page }) => {
        await page.goto('/kicad/pcbnew.html');
    });

    test('click through setup wizard to load PCBnew', async ({ page, testLogger }) => {
        // Wait for main canvas to be visible (KiCad takes time to initialize)
        await expect(page.locator('#canvas')).toBeVisible({ timeout: 90000 });
        await page.waitForTimeout(2000);

        // Screenshot initial wizard state
        await page.screenshot({ path: 'test-results/wizard-00-initial.png' });

        // Click through wizard - try Next >, then Finish if not found
        for (let i = 1; i <= 10; i++) {
            // Dump buttons before clicking to see what's available
            console.log(`Step ${i}: Looking for buttons...`);
            await dumpElements(page);

            // Try "Next >" first (exact button label)
            let clicked = await clickByLabel(page, 'Next >');
            if (clicked) {
                console.log(`Step ${i}: Clicked "Next >"`);
            } else {
                // Try Finish button
                clicked = await clickByLabel(page, 'Finish');
                if (clicked) {
                    console.log(`Step ${i}: Clicked "Finish"`);
                    await page.waitForTimeout(500);
                    await page.screenshot({ path: `test-results/wizard-${String(i).padStart(2, '0')}-finish.png` });
                    break;
                } else {
                    console.log(`Step ${i}: No "Next >" or "Finish" found`);
                    break;
                }
            }
            await page.waitForTimeout(500);
            await page.screenshot({ path: `test-results/wizard-${String(i).padStart(2, '0')}.png` });
        }

        // Wait for PCBnew to fully load
        await page.waitForTimeout(2000);
        await page.screenshot({ path: 'test-results/pcbnew-loaded.png' });

        // Verify PCBnew loaded
        const canvasCount = await page.locator('canvas').count();
        console.log('Canvas count after wizard:', canvasCount);
        expect(canvasCount).toBeGreaterThan(0);
    });
});
