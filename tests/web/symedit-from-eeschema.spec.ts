import { test, expect, type Page } from '@playwright/test';
import { clickByTooltip, waitForWxApp } from '../e2e/utils/element-tracker';

/**
 * Symbol Editor opened from a schematic session must show its libraries.
 *
 * Companion to `fpedit-from-eeschema.spec.ts`, and deliberately the WEAKER of the
 * two: this path is expected to be green already. It is here because
 * `SYMBOL_EDIT_FRAME::SyncLibraries()` carries the same latent defect shape the
 * footprint side had —
 *
 *     SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &Prj() );
 *     adapter->BlockUntilLoaded();          // ← no AsyncLoad() first
 *
 * — which `FOOTPRINT_LIST_IMPL::ReadFootprintFiles()` explicitly warns against
 * ("AsyncLoad() must be called before BlockUntilLoaded() to ensure library
 * loading is started"). `BlockUntilLoaded()` on a face that never preloaded
 * returns immediately with nothing loaded, and the tree comes up empty.
 *
 * It cannot fail TODAY because every route to the Symbol Editor already has
 * FACE_SCH running: `--frame=symedit` preloads it, and the only UI entry point is
 * eeschema's own toolbar (`ACTIONS::showSymbolEditor` is appended solely by
 * toolbars_sch_editor.cpp:201 — nothing in pcbnew/ references it). So this spec
 * pins the currently-working behaviour; it turns into a real regression guard the
 * moment a cross-kiface entry point is added.
 *
 * Same two traps as the footprint spec: assert on the wx FRAME LIST, not
 * `document.title` (which stays "… — Schematic Editor" even after a second editor
 * opens), and use the fileless `-/eeschema` route so no apps/sync is required.
 */

const SCOPE = 'default';
const BOOT_TIMEOUT = 180000;

function frameNames(page: Page): Promise<string[]> {
  return page.evaluate(() =>
    (window as unknown as { wxElementRegistry: any }).wxElementRegistry
      .findAll({})
      .filter((e: any) => /Frame$/.test(e.typeName || ''))
      .map((e: any) => e.name as string),
  );
}

function treeRowCount(page: Page): Promise<number> {
  return page.evaluate(
    () =>
      (window as unknown as { wxElementRegistry: any }).wxElementRegistry.findAllRendered({
        elementType: 'dataviewitem',
      }).length,
  );
}

test.describe('symbol editor reached from eeschema', () => {
  test.describe.configure({ timeout: 420000 });

  test('opens with a populated library tree', async ({ page }) => {
    const aborts: string[] = [];
    page.on('console', (m) => {
      if (/Aborted\(/i.test(m.text())) aborts.push(m.text());
    });

    await page.goto(`/${SCOPE}/projects/demo/-/eeschema`);
    await waitForWxApp(page, { timeout: BOOT_TIMEOUT });
    await expect
      .poll(() => page.title(), { message: 'schematic editor up', timeout: BOOT_TIMEOUT })
      .toMatch(/Schematic Editor/i);
    await page.waitForFunction(
      () => !!(window as unknown as { kicadLibs?: unknown }).kicadLibs,
      null,
      { timeout: 60000 },
    );

    expect(await frameNames(page)).toEqual(['SchematicFrame']);

    // Settle the baseline before reading it — see the footprint spec's header.
    let baseline = -1;
    await expect
      .poll(
        async () => {
          const n = await treeRowCount(page);
          const settled = n === baseline;
          baseline = n;
          return settled;
        },
        { message: 'schematic tree row count settled', timeout: 60000, intervals: [1000] },
      )
      .toBe(true);

    expect(
      await clickByTooltip(page, 'Create, delete and edit schematic symbols', {
        elementType: 'tool',
      }),
      'Symbol Editor toolbar button found and clicked',
    ).toBe(true);

    await expect
      .poll(() => frameNames(page), {
        message: 'Symbol Editor frame (LibeditFrame) opened',
        timeout: BOOT_TIMEOUT,
      })
      .toContain('LibeditFrame');

    await expect
      .poll(() => treeRowCount(page), {
        message: `symbol library tree rows rendered (baseline was ${baseline})`,
        timeout: 120000,
      })
      .toBeGreaterThan(baseline);

    expect(aborts, 'no WASM abort').toEqual([]);
  });
});
