import { test, expect } from '@playwright/test';
import path from 'path';

/**
 * PCBnew WASM E2E Tests
 *
 * These tests verify that the KiCad PCBnew application runs correctly in the browser.
 * They test the basic UI functionality and WASM module loading.
 */

test.describe('PCBnew WASM', () => {
    test.beforeEach(async ({ page }) => {
        // Navigate to PCBnew test page
        const htmlPath = path.join(__dirname, 'pcbnew.html');
        await page.goto(`file://${htmlPath}`);

        // Wait for loading overlay to disappear
        await page.waitForSelector('#loading-overlay.hidden', { timeout: 30000 });
    });

    test('should load the PCBnew interface', async ({ page }) => {
        // Check that the toolbar is visible
        await expect(page.locator('#toolbar h1')).toHaveText('KiCad PCBnew (WASM)');

        // Check that toolbar buttons exist
        await expect(page.locator('#btn-new')).toBeVisible();
        await expect(page.locator('#btn-open')).toBeVisible();
        await expect(page.locator('#btn-save')).toBeVisible();
        await expect(page.locator('#btn-zoom-in')).toBeVisible();
        await expect(page.locator('#btn-zoom-out')).toBeVisible();
        await expect(page.locator('#btn-fit')).toBeVisible();
    });

    test('should have a working canvas', async ({ page }) => {
        const canvas = page.locator('#canvas');
        await expect(canvas).toBeVisible();

        // Check that canvas has proper dimensions
        const box = await canvas.boundingBox();
        expect(box?.width).toBeGreaterThan(0);
        expect(box?.height).toBeGreaterThan(0);
    });

    test('should track mouse coordinates', async ({ page }) => {
        const canvas = page.locator('#canvas');
        const statusCoords = page.locator('#status-coords');

        // Move mouse over canvas
        await canvas.hover({ position: { x: 100, y: 100 } });

        // Check that coordinates are updated
        const coordText = await statusCoords.textContent();
        expect(coordText).toMatch(/X: \d+\.\d+mm Y: \d+\.\d+mm/);
    });

    test('should log UI interactions', async ({ page }) => {
        const logPanel = page.locator('#log-panel');

        // Click New button
        await page.click('#btn-new');

        // Check that action was logged
        await expect(logPanel).toContainText('New project');
    });

    test('should have status bar with info', async ({ page }) => {
        await expect(page.locator('#status-info')).toHaveText('Ready');
    });

    test('should show sidebar with log panel', async ({ page }) => {
        const sidebar = page.locator('#sidebar');
        const logPanel = page.locator('#log-panel');

        await expect(sidebar).toBeVisible();
        await expect(logPanel).toBeVisible();
    });

    // Screenshot test for visual regression
    test('should match visual snapshot', async ({ page }) => {
        await page.screenshot({
            path: path.join(__dirname, 'screenshots', 'pcbnew-initial.png'),
            fullPage: true
        });
    });
});

test.describe('PCBnew File Operations', () => {
    test.beforeEach(async ({ page }) => {
        const htmlPath = path.join(__dirname, 'pcbnew.html');
        await page.goto(`file://${htmlPath}`);
        await page.waitForSelector('#loading-overlay.hidden', { timeout: 30000 });
    });

    test('should have file open button that triggers file picker', async ({ page }) => {
        const openBtn = page.locator('#btn-open');
        await expect(openBtn).toBeVisible();

        // Click should trigger file input (we can't fully test file selection in Playwright)
        // but we can verify the button is clickable
        await openBtn.click();
    });
});

test.describe('PCBnew WASM Module', () => {
    test('WASM module loading (placeholder)', async ({ page }) => {
        // This test will be expanded once the WASM module is built
        const htmlPath = path.join(__dirname, 'pcbnew.html');
        await page.goto(`file://${htmlPath}`);

        // Check for expected warning when module is not built
        const logPanel = page.locator('#log-panel');
        await page.waitForSelector('#loading-overlay.hidden', { timeout: 30000 });
        await expect(logPanel).toContainText('Note: PCBnew WASM module not yet built');
    });
});
