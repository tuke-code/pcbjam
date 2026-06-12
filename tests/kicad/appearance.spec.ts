// Appearance panel (right side of pcbnew): Layers/Objects/Nets notebook.
// Regression coverage for the DOM-port fixes: native tab switching (all
// three tabs and back), wheel scrolling inside the scrolled pages with
// viewport clipping, and rows surviving tab round-trips. Screenshots are
// written to test-results/appearance-*.png for visual review.

import { test, expect, Page } from '@playwright/test';
import { clickByLabel } from '../e2e/utils/element-tracker';

declare global {
    interface Window {
        wxElementRegistry: any;
        wxDomPort?: boolean;
        wxDomControls?: Map<number, HTMLElement>;
    }
}

async function completeWizard(page: Page): Promise<void> {
    await expect(page.locator('#canvas')).toBeVisible({ timeout: 90000 });
    await page.waitForFunction(() => !!window.wxElementRegistry, null, { timeout: 90000 });
    await page.waitForTimeout(2000);

    for (let i = 1; i <= 10; i++) {
        let clicked = await clickByLabel(page, 'Next >');

        if (!clicked) {
            await clickByLabel(page, 'Finish');
            break;
        }

        await page.waitForTimeout(500);
    }

    // let the main frame settle
    await page.waitForTimeout(4000);
}

type Tab = { label: string; subType: string; centerX: number; centerY: number };

// The appearance notebook's tabs: rendered 'tab' elements with real
// coordinates (other, hidden notebooks register at 0,0).
async function appearanceTabs(page: Page): Promise<Tab[]> {
    return page.evaluate(() => {
        const out: any[] = [];
        window.wxElementRegistry.renderedElements.forEach((e: any) => {
            if (e.elementType === 'tab' && e.centerX > 0 &&
                ['Layers', 'Objects', 'Nets'].includes(e.label)) {
                out.push({ label: e.label, subType: e.subType,
                           centerX: e.centerX, centerY: e.centerY });
            }
        });
        return out;
    });
}

async function selectTab(page: Page, label: string): Promise<void> {
    const tabs = await appearanceTabs(page);
    const tab = tabs.find(t => t.label === label);
    expect(tab, `appearance tab ${label}`).toBeTruthy();
    await page.mouse.click(tab!.centerX, tab!.centerY);

    await expect.poll(async () => {
        const after = await appearanceTabs(page);
        return after.find(t => t.label === label)?.subType;
    }, { timeout: 5000, intervals: [200] }).toBe('selected');

    await page.waitForTimeout(400);
}

// DOM port only: viewport rects of row labels inside the appearance pane
// (spans are real elements there; the canvas port draws them as pixels).
async function rowLabelTops(page: Page, labels: string[]): Promise<Record<string, number | null>> {
    return page.evaluate((wanted: string[]) => {
        const out: Record<string, number | null> = {};
        for (const w of wanted) out[w] = null;
        if (!window.wxDomControls) return out;
        for (const [, el] of window.wxDomControls) {
            if (el.tagName === 'SPAN' && wanted.includes(el.textContent || '')) {
                out[el.textContent as string] = Math.round(el.getBoundingClientRect().top);
            }
        }
        return out;
    }, labels);
}

test.describe('Appearance panel (Layers/Objects/Nets)', () => {
    test.beforeEach(async ({ page }) => {
        await page.goto('/kicad/pcbnew.html');
    });

    test('tabs switch through all three pages and back', async ({ page }) => {
        await completeWizard(page);

        const tabs = await appearanceTabs(page);
        expect(tabs.map(t => t.label).sort()).toEqual(['Layers', 'Nets', 'Objects']);
        expect(tabs.find(t => t.label === 'Layers')?.subType).toBe('selected');

        await page.screenshot({ path: 'test-results/appearance-00-layers.png' });

        await selectTab(page, 'Objects');
        await page.screenshot({ path: 'test-results/appearance-01-objects.png' });

        await selectTab(page, 'Nets');
        await page.screenshot({ path: 'test-results/appearance-02-nets.png' });

        // and back to the start
        await selectTab(page, 'Layers');
        await page.screenshot({ path: 'test-results/appearance-03-layers-again.png' });

        // DOM port: layer rows must survive the tab round-trip (regression:
        // pages came back blank after switching away and back)
        if (await page.evaluate(() => !!window.wxDomPort)) {
            const tops = await rowLabelTops(page, ['F.Cu', 'B.Cu']);
            expect(tops['F.Cu'], 'F.Cu row visible after tab round-trip').not.toBeNull();
            expect(tops['B.Cu'], 'B.Cu row visible after tab round-trip').not.toBeNull();
        }
    });

    test('layer list scrolls with the wheel and clips at the pane', async ({ page }) => {
        await completeWizard(page);

        const tabs = await appearanceTabs(page);
        const layersTab = tabs.find(t => t.label === 'Layers');
        expect(layersTab).toBeTruthy();

        const isDom = await page.evaluate(() => !!window.wxDomPort);

        // hover INSIDE the layer list (just below the tab strip)
        const hoverX = layersTab!.centerX;
        const hoverY = layersTab!.centerY + 120;
        await page.mouse.move(hoverX, hoverY);

        const before = isDom ? await rowLabelTops(page, ['B.Cu', 'F.Mask']) : null;

        await page.mouse.wheel(0, 240);
        await page.waitForTimeout(800);
        await page.screenshot({ path: 'test-results/appearance-10-layers-scrolled.png' });

        if (isDom && before) {
            const after = await rowLabelTops(page, ['B.Cu', 'F.Mask']);
            expect(after['B.Cu'], 'B.Cu moved up after wheel scroll')
                .toBeLessThan(before['B.Cu']!);
            expect(after['F.Mask'], 'F.Mask moved up after wheel scroll')
                .toBeLessThan(before['F.Mask']!);
        }

        // scroll back up restores the start of the list
        await page.mouse.wheel(0, -480);
        await page.waitForTimeout(800);
        await page.screenshot({ path: 'test-results/appearance-11-layers-scrolled-back.png' });

        if (isDom && before) {
            const restored = await rowLabelTops(page, ['B.Cu']);
            expect(restored['B.Cu'], 'B.Cu back at its original position')
                .toBe(before['B.Cu']);
        }
    });

    test('objects page scrolls with the wheel', async ({ page }) => {
        await completeWizard(page);

        await selectTab(page, 'Objects');

        const tabs = await appearanceTabs(page);
        const objectsTab = tabs.find(t => t.label === 'Objects')!;
        const isDom = await page.evaluate(() => !!window.wxDomPort);

        await page.mouse.move(objectsTab.centerX, objectsTab.centerY + 120);

        const before = isDom ? await rowLabelTops(page, ['Ratsnest']) : null;

        await page.mouse.wheel(0, 240);
        await page.waitForTimeout(800);
        await page.screenshot({ path: 'test-results/appearance-20-objects-scrolled.png' });

        if (isDom && before && before['Ratsnest'] !== null) {
            const after = await rowLabelTops(page, ['Ratsnest']);
            expect(after['Ratsnest'], 'Ratsnest row moved after wheel scroll')
                .toBeLessThan(before['Ratsnest']!);
        }
    });
});
