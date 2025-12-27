import { test, expect } from '@playwright/test';

/**
 * PCBnew WASM E2E Tests
 *
 * These tests verify that the KiCad PCBnew application runs correctly in the browser.
 * The WASM files are served from apps/kicad/ via the web server.
 */

test.describe('PCBnew WASM', () => {
    test.beforeEach(async ({ page }) => {
        // Navigate to PCBnew via web server (apps/kicad/pcbnew.html)
        await page.goto('/kicad/pcbnew.html');
    });

    test('should load PCBnew WASM module', async ({ page }) => {
        // Wait for WASM to initialize (look for canvas or status indicator)
        // This is a basic smoke test - expand as PCBnew integration matures
        await expect(page.locator('canvas')).toBeVisible({ timeout: 60000 });
    });

    test('should render without JavaScript errors', async ({ page }) => {
        const errors: string[] = [];
        page.on('pageerror', err => errors.push(err.message));

        await page.goto('/kicad/pcbnew.html');
        await page.waitForTimeout(5000);  // Give time for WASM to load

        // Filter out known acceptable errors if any
        const criticalErrors = errors.filter(e => !e.includes('ResizeObserver'));
        expect(criticalErrors).toHaveLength(0);
    });
});
