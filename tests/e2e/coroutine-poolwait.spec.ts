import { test, expect, tryLoadApp } from './utils/fixtures';

// REPRO of the native-EH collab-apply thread-pool DEADLOCK (task #54). The real KiCad pool's
// submit_task + wait_for(250ms) poll loop (the connectivity/searchConnections idiom) run from a
// wxEvtHandler::CallAfter — i.e. INSIDE the established per-frame-yield wx main loop, exactly like
// kicadCollabApply -> doApply -> commit.Push -> RecalculateRatsnest. The control (?defer=0) runs the
// SAME work in OnInit, before the main loop exists (the board-LOAD path, which works in KiCad).
//
// 4-agent research conclusion: the wait is a main-thread futex busy-spin (pthread_cond_wait ->
// emscripten_futex_wait -> _emscripten_yield), NOT an asyncify suspend; from inside the rAF main
// loop it pumps only the proxy queue, never the JS event loop, so a worker that needs the event
// loop never completes -> permanent spin. Expectation BEFORE a fix: control GREEN, repro RED (the
// 'AFTER emscripten_sleep' line proves asyncify itself is fine; only the futex pool wait hangs).
test.describe('pool-callafter: thread-pool wait from a CallAfter (collab-apply deadlock repro)', () => {
  test.beforeEach(async ({ page }) => {
    page.on('console', (m) => console.log('[PAGE]', m.text()));
  });
  test('CONTROL ?defer=0 (OnInit / board-LOAD path) — pool wait completes', async ({ page, testLogger }) => {
    await page.goto('/standalone/pool-callafter/pool_callafter_test.html?defer=0');
    await tryLoadApp(page, 20000).catch(() => {});
    await expect
      .poll(() => testLogger.consoleLogs.some((l) => l.includes('[POOL] SUCCESS')), {
        timeout: 40000,
        message: 'control (OnInit, before the main loop) should complete the pool wait',
      })
      .toBe(true);
  });

  test('REPRO ?defer=1 (CallAfter / collab-APPLY path) — pool wait must complete', async ({ page, testLogger }) => {
    await page.goto('/standalone/pool-callafter/pool_callafter_test.html?defer=1');
    await tryLoadApp(page, 20000).catch(() => {});

    // The CallAfter fires and a PLAIN asyncify suspend rewinds there — isolates the asyncify
    // mechanism (healthy) from the futex pool wait (the suspect).
    await expect
      .poll(() => testLogger.consoleLogs.some((l) => l.includes('AFTER emscripten_sleep')), {
        timeout: 20000,
        message: 'the CallAfter should fire and emscripten_sleep should rewind in that context',
      })
      .toBe(true);

    // The decisive check: does the real-pool futex wait complete from the CallAfter, or deadlock?
    await expect
      .poll(() => testLogger.consoleLogs.some((l) => l.includes('[POOL] SUCCESS')), {
        timeout: 40000,
        message: 'REPRO: the pool wait from a CallAfter must complete (RED = the deadlock is reproduced)',
      })
      .toBe(true);
  });
});
