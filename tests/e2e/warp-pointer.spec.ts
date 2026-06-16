import { test, expect, tryLoadApp } from './utils/fixtures';

// Red-green reproduction for the wxWindow::WarpPointer no-op (src/wasm/window.cpp).
//
// WarpPointer() was a no-op because the browser cannot move the OS pointer. But
// KiCad's arrow-key cursor nudge warps the pointer and then reads it back via
// wxGetMousePosition() (SetCursorPosition -> WarpMouseCursor -> WarpPointer, then
// the interactive-move loop reads GetMousePosition()). With the warp dead the
// read is stale, so a moved item never follows the arrow keys and snaps to the
// cursor on grab (pcbnew issue #9). The standalone app warps to known points and
// self-reports whether wxGetMousePosition() tracked them.
//
//   RED  (bug present): wxGetMousePosition() unchanged by the warp.
//   GREEN (fixed):      wxGetMousePosition() == ClientToScreen({x, y}).

function reproLine(logs: string[], name: string): string | undefined {
  return logs.find((l) => l.includes(`[REPRO] ${name}:`));
}

test.describe('wxWindow::WarpPointer cached mouse position', () => {
  test('WarpPointer updates wxGetMousePosition()', async ({ page, testLogger }) => {
    const name = 'warppointer_updates_position';
    await page.goto('/standalone/warp-pointer/warp-pointer_test.html');
    expect(await tryLoadApp(page, 30000), 'repro app should load').toBe(true);

    await expect
      .poll(() => reproLine(testLogger.consoleLogs, name) ?? null, {
        timeout: 30000,
        message: `repro app should emit its [REPRO] ${name} result line`,
      })
      .not.toBeNull();

    const line = reproLine(testLogger.consoleLogs, name)!;
    expect(line, `repro line was: ${line}`).toContain(`[REPRO] ${name}: PASS`);
  });
});
