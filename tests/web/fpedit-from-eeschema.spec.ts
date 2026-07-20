import { test, expect, type Page } from '@playwright/test';
import { clickByTooltip, stableShot, waitForWxApp } from '../e2e/utils/element-tracker';

/**
 * Footprint Editor opened FROM a schematic session must show its libraries.
 *
 * Regression guard for the ysync-0010 §4 bug: `single_top.cpp` preloads libraries
 * only for the face the app booted into. Booting `--frame=fpedit` (what
 * `-/footprint_editor` does, and what every other fp-editor spec exercises)
 * preloads FACE_PCB and works. But the eeschema toolbar button routes through
 * `Kiway().Player( FRAME_FOOTPRINT_EDITOR )`, which starts FACE_PCB lazily at
 * click time and never preloads — so nothing had walked the footprint libraries,
 * `LIBRARY_MANAGER_ADAPTER::GetLibraryNames()` reported only rows whose status is
 * LOADED (i.e. none), and `FP_TREE_SYNCHRONIZING_ADAPTER::Sync` skipped
 * everything. The editor opened with a completely empty library tree.
 *
 * Fixed by `AsyncLoad()` + `BlockUntilLoaded()` in
 * `FOOTPRINT_EDIT_FRAME::initLibraryTree()`.
 *
 * Two things this spec must NOT do, both learned the hard way:
 *
 *  - Don't assert on `document.title`. It stays "… — Schematic Editor" even after
 *    a second editor frame opens (verified with Symbol Editor too), so a title
 *    poll reports failure on a working transition. The wx FRAME LIST is the
 *    signal: `ModEditFrame` appearing IS the Footprint Editor opening.
 *  - Don't use the `demo.kicad_sch` route. It boots its document from the Y.Doc
 *    and needs apps/sync (:3055), which this suite's stack does not run; the doc
 *    stalls at "untitled [Unsaved]" and FACE_PCB never starts. The fileless
 *    `-/eeschema` route needs no sync, and nothing here needs a loaded schematic —
 *    the bug is about which kiface preloaded its libraries.
 *
 * The tree assertion is a DELTA, not an absolute count: the schematic editor
 * already renders its own dataviewitem rows (symbol libs), so "> 0" would pass
 * even with a totally empty footprint tree. Growth past that baseline is exactly
 * "the footprint tree got rows".
 *
 * Determinism: no blind waits — readiness comes from waitForWxApp, the app's own
 * `kicadLibs` observable, and polling for the state each step produces.
 */

const SCOPE = 'default';
const BOOT_TIMEOUT = 180000;

/** Names of the live wx top-level frames. */
function frameNames(page: Page): Promise<string[]> {
  return page.evaluate(() =>
    (window as unknown as { wxElementRegistry: any }).wxElementRegistry
      .findAll({})
      .filter((e: any) => /Frame$/.test(e.typeName || ''))
      .map((e: any) => e.name as string),
  );
}

/** Rendered library-tree rows across whatever frames are up. */
function treeRowCount(page: Page): Promise<number> {
  return page.evaluate(
    () =>
      (window as unknown as { wxElementRegistry: any }).wxElementRegistry.findAllRendered({
        elementType: 'dataviewitem',
      }).length,
  );
}

test.describe('footprint editor reached from eeschema', () => {
  test.describe.configure({ timeout: 420000 });

  test('opens with a populated library tree', async ({ page }) => {
    const aborts: string[] = [];
    page.on('console', (m) => {
      if (/Aborted\(/i.test(m.text())) aborts.push(m.text());
    });

    // 1) Boot the SCHEMATIC editor — the face whose preload does not cover footprints.
    await page.goto(`/${SCOPE}/projects/demo/-/eeschema`);
    await waitForWxApp(page, { timeout: BOOT_TIMEOUT });
    await expect
      .poll(() => page.title(), { message: 'schematic editor up', timeout: BOOT_TIMEOUT })
      .toMatch(/Schematic Editor/i);

    // The libs bridge must exist before we cross over, or a miss below would be an
    // app-plumbing failure rather than the kiface-preload bug under test.
    await page.waitForFunction(
      () => !!(window as unknown as { kicadLibs?: unknown }).kicadLibs,
      null,
      { timeout: 60000 },
    );

    expect(await frameNames(page)).toEqual(['SchematicFrame']);

    // Baseline: the schematic editor's OWN tree rows (symbol libs) — see header.
    // It must be SETTLED before we read it: those rows render asynchronously, and
    // a baseline sampled too early (0) would weaken the delta below into "> 0",
    // which the schematic's own rows could satisfy on their own. Two consecutive
    // equal reads = settled.
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

    // 2) Cross to the Footprint Editor the way a user does: the toolbar button
    //    (ACTIONS::showFootprintEditor → Kiway().Player( FRAME_FOOTPRINT_EDITOR )).
    expect(
      await clickByTooltip(page, 'Create, delete and edit board footprints', {
        elementType: 'tool',
      }),
      'Footprint Editor toolbar button found and clicked',
    ).toBe(true);

    // FACE_PCB starts lazily here — this is the first time the pcbnew kiface runs.
    await expect
      .poll(() => frameNames(page), {
        message: 'Footprint Editor frame (ModEditFrame) opened',
        timeout: BOOT_TIMEOUT,
      })
      .toContain('ModEditFrame');

    // 3) THE ASSERTION: its library tree is populated. Before the fix the frame
    //    opened exactly like this and the tree stayed empty, so the row count
    //    never moved off the schematic editor's baseline.
    await expect
      .poll(() => treeRowCount(page), {
        message: `footprint library tree rows rendered (baseline was ${baseline})`,
        timeout: 120000,
      })
      .toBeGreaterThan(baseline);

    await stableShot(page, 'fpedit-from-eeschema-tree.png');
    expect(aborts, 'no WASM abort').toEqual([]);
  });
});
