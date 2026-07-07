import { test, expect } from './fixtures';
import { clickMenuBarItem } from '../e2e/utils/element-tracker';
import type { Page } from '@playwright/test';

// KiCad-level reproduction of the wxWidgets DOM-port "menu enable/check goes
// stale" bug (parity audit H-7), fixed in wxwidgets/src/wasm/menu.cpp +
// domevents.cpp + include/wx/wasm/{window,menu}.h + build/wasm/wx-dom.js.
//
// Native behavior: opening a menu fires wxEVT_MENU_OPEN, and KiCad refreshes
// each item's enable/check state just-in-time (ACTION_MENU::OnMenuEvent runs
// ACTIONS::updateMenu, gated on wxEVT_MENU_OPEN — action_menu.cpp:421-426).
//
// In the WASM/DOM port that event is NEVER fired: clicking a menubar title
// opens the popup from a cached JS snapshot (build/wasm/wx-dom.js) and never
// calls back into C++. The menu is only re-serialized to the DOM on structural
// mutations (Append/Insert/Remove) — so every item keeps the enable/check
// state it had at construction/attach time. wxUSE_IDLEMENUUPDATES==1 refreshes
// the C++ item state on idle, but nothing pushes it to the DOM. KiCad's entire
// menu enable/check refresh is therefore dead.
//
// Surface: Edit ▸ Undo. Undo is disabled while the undo stack is empty and must
// enable after an undoable action. We load a board (empty undo stack), read the
// Undo item's enabled flag, perform one real BOARD_COMMIT move via the embind
// hook (pushes one undo entry), then re-read.
//
//   RED  (bug present): the Undo item is frozen at its construction default
//                       (enabled=true) and never tracks the undo stack — so it
//                       reads enabled=true even with an empty stack, and does
//                       not change after the action.
//   GREEN (fixed):      Undo reads enabled=false with an empty stack and
//                       enabled=true after the move — it tracks the stack.

const DOC_DIR = `/home/kicad/documents`;
const PCB_PATH = `${DOC_DIR}/menu_undo.kicad_pcb`;

// A minimal board with one footprint (the item kicadCollabTestMoveFirst moves,
// proven in save-hook.spec.ts / pcbnew-collab.spec.ts). The move runs through a
// BOARD_COMMIT::Push → one undo entry. Loading a board does not push undo
// entries, so the undo stack is empty until the move.
const BOARD = `(kicad_pcb
	(version 20241229)
	(generator "pcbnew")
	(generator_version "9.0")
	(general (thickness 1.6))
	(paper "A4")
	(layers
		(0 "F.Cu" signal)
		(2 "B.Cu" signal)
		(37 "F.SilkS" user)
		(25 "Edge.Cuts" user)
	)
	(setup)
	(net 0 "")
	(footprint "TestLib:R"
		(layer "F.Cu")
		(uuid "99999999-0000-0000-0000-000000000001")
		(at 100 100)
		(attr smd)
		(property "Reference" "R1" (at 0 -4.2 0) (layer "F.SilkS") (uuid "99999999-0000-0000-0000-0000000000aa") (effects (font (size 1 1) (thickness 0.15))))
	)
)
`;

// Boot the seeded pcbnew-collab harness (skips the first-run wizard, and is the
// proven context for kicadCollabTestMoveFirst — save-hook/pcbnew-collab specs).
async function bootPcbnew(page: Page): Promise<void> {
  await expect(page.locator('#canvas')).toBeVisible({ timeout: 90000 });
  await page.waitForFunction(() => !!window.wxElementRegistry, null, { timeout: 90000 });
  await page.waitForFunction(
    () => {
      const m = (window as unknown as { Module?: Record<string, unknown> }).Module;
      return (
        typeof m?.kicadOpenFile === 'function' &&
        typeof m?.kicadCollabTestMoveFirst === 'function' &&
        typeof m?.kicadCollabGetPos === 'function'
      );
    },
    null,
    { timeout: 90000 },
  );
  await page.waitForFunction(
    () =>
      !!window.wxElementRegistry &&
      window.wxElementRegistry
        .findAll({ visible: true })
        .some((e) => /Frame$/.test(e.typeName) || (e.name || '').endsWith('Frame')),
    null,
    { timeout: 90000 },
  );
}

// A click on the GAL canvas pumps the wasm main loop so deferred work
// (kicadCollabTestMoveFirst queues its BOARD_COMMIT via CallAfter+coroutine)
// actually drains — see save-hook.spec.ts's focusCanvas.
async function pumpCanvas(page: Page): Promise<void> {
  const box = await page.locator('#canvas').boundingBox();
  if (box) await page.mouse.click(box.x + box.width / 2, box.y + box.height / 2);
  await page.waitForTimeout(400);
}

async function openBoard(page: Page): Promise<void> {
  await page.evaluate(
    ({ dir, path, content }) => {
      const w = window as unknown as {
        FS: { mkdirTree(p: string): void; writeFile(p: string, d: string): void };
        Module: { kicadOpenFile(p: string): unknown };
      };
      try {
        w.FS.mkdirTree(dir);
      } catch {
        /* exists */
      }
      w.FS.writeFile(path, content);
      w.Module.kicadOpenFile(path);
    },
    { dir: DOC_DIR, path: PCB_PATH, content: BOARD },
  );
}

// Open the Edit menu, read the Undo item's enabled flag from the rendered
// registry, then close the menu again.
async function readUndoEnabled(page: Page): Promise<boolean | null> {
  expect(await clickMenuBarItem(page, 'Edit'), 'Edit menu should open').toBe(true);
  await page.waitForTimeout(700);
  const enabled = await page.evaluate(() => {
    const r = window.wxElementRegistry;
    const items = (r?.findAllRendered?.({}) ?? []).filter((e) => e.elementType === 'menuitem');
    const undo = items.find((e) => /^Undo\b/.test(e.label || ''));
    return undo ? undo.enabled : null;
  });
  await page.keyboard.press('Escape');
  await page.waitForTimeout(400);
  return enabled;
}

test.describe('pcbnew Edit menu — Undo enable tracks the undo stack (H-7)', () => {
  test.describe.configure({ timeout: 240000 });

  test('Undo enable state refreshes when the menu opens', async ({ page, testLogger }) => {
    await page.goto('/kicad/pcbnew-collab.html');
    await bootPcbnew(page);

    await openBoard(page);
    await page.waitForTimeout(3000);
    await page.screenshot({ path: 'test-results/menu-undo-00-loaded.png', scale: 'device' });

    // --- baseline: empty undo stack ---
    const E0 = await readUndoEnabled(page);
    console.log(`[H7] Undo.enabled with an empty undo stack: ${E0}`);
    expect(E0, 'Undo menu item should be registered').not.toBeNull();

    // --- undoable action: move the first item via a real BOARD_COMMIT ---
    const movedId = await page.evaluate(() =>
      (window as unknown as { Module: { kicadCollabTestMoveFirst(dx: number, dy: number): string } })
        .Module.kicadCollabTestMoveFirst(2_000_000, 0),
    );
    expect(movedId, 'an item should have been picked to move').toBeTruthy();

    // The move is deferred (CallAfter + coroutine); a canvas click pumps the
    // wasm main loop so the BOARD_COMMIT::Push actually runs (dirtying the
    // board → one undo entry, as save-hook.spec.ts relies on). Pump a few times
    // to be sure the deferred work drained.
    for (let i = 0; i < 3; i++) await pumpCanvas(page);
    console.log(`[H7] moved item ${movedId}; board dirtied via BOARD_COMMIT`);

    // --- post-action ---
    const E1 = await readUndoEnabled(page);
    console.log(`[H7] Undo.enabled after an undoable move: ${E1}`);
    await page.screenshot({ path: 'test-results/menu-undo-01-after.png', scale: 'device' });

    const aborted = [...testLogger.consoleLogs, ...testLogger.errors].some((l) =>
      l.includes('Aborted('),
    );
    expect(aborted, 'WASM module should not abort').toBe(false);

    // RED trap: the frozen construction default is enabled=true, so an empty
    // undo stack must read false only when the menu re-serializes on open.
    expect(E0, 'Undo should be DISABLED with an empty undo stack').toBe(false);
    expect(E1, 'Undo should be ENABLED after an undoable action').toBe(true);
    expect(E1, 'Undo enable state must change with the undo stack').not.toBe(E0);
  });
});
