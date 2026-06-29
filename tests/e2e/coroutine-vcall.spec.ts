import { test, expect, tryLoadApp } from './utils/fixtures';

// ISOLATION repro for task #54: which virtual call_indirect SIGNATURES park under native-EH + asyncify,
// called from a libcontext fiber. Logs getInt(ii) / setViii(viii) / getVec(sret-vii) / setVii(vii).
// If only ii + viii print their "after"/"OK" line and the vii ones don't, the hang is signature-specific.
test.describe('vcall-fiber: signature-specific call_indirect mis-dispatch (native-EH)', () => {
  test.beforeEach(async ({ page }) => {
    page.on('console', (m) => console.log('[PAGE]', m.text()));
  });

  test('ii / viii / sret-vii / vii from a fiber', async ({ page, testLogger }) => {
    await page.goto('/standalone/coroutine-pthread/vcall_fiber_repro.html');
    await tryLoadApp(page, 20000).catch(() => {});
    await page.waitForTimeout(8000);
    const logs = testLogger.consoleLogs.filter((l) => l.includes('[VCALL]'));
    console.log('=== VCALL SIGNATURE TRACE ===\n' + logs.join('\n') + '\n=== END ===');
    // Informational: did the run reach DONE (all signatures passed) or park?
    const reachedDone = logs.some((l) => l.includes('DONE'));
    console.log('reachedDONE=' + reachedDone);
  });

  test('rAF main-loop: ii / viii / sret-vii / vii from the COROUTINE apply', async ({ page, testLogger }) => {
    await page.goto('/standalone/coroutine-pthread/vcall_mainloop_repro.html');
    await tryLoadApp(page, 20000).catch(() => {});
    await page.waitForTimeout(9000);
    const logs = testLogger.consoleLogs.filter((l) => l.includes('[VCALL]'));
    console.log('=== MAINLOOP VCALL TRACE ===\n' + logs.join('\n') + '\n=== END ===');
    console.log('mainloop reachedDONE=' + logs.some((l) => l.includes('DONE')));
  });

  test('eh-loop: try/catch_all + RAII + suspend + loop around the four signatures', async ({ page, testLogger }) => {
    await page.goto('/standalone/coroutine-pthread/vcall_ehloop_repro.html');
    await tryLoadApp(page, 20000).catch(() => {});
    await page.waitForTimeout(9000);
    const logs = testLogger.consoleLogs.filter((l) => l.includes('[VCALL]'));
    console.log('=== EH-LOOP VCALL TRACE ===\n' + logs.join('\n') + '\n=== END ===');
    console.log('ehloop reachedDONE=' + logs.some((l) => l.includes('DONE')));
  });
});
