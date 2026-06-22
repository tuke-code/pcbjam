import { test, expect } from '../e2e/utils/fixtures';

// Red-green-fixed spec for the native-wasm-EH spike toy
// (tests/apps/standalone/eh-spike/eh_spike_test.cpp — see
// docs/features/wasm-exceptions/06-spike-plan.md, Phases 1 & 1.5).
//
// Built three ways from one source (scripts/build-eh-spike.sh):
//   - eh_spike_jseh         : -fexceptions          JS-EH baseline                → all green
//   - eh_spike_wasmeh        : -fwasm-exceptions =1   native wasm-EH, no pass       → suspend-in-catch TRAPS
//   - eh_spike_wasmeh_hoist  : -fwasm-exceptions =1   native wasm-EH + hoist pass   → all green
//
// This pins both the Binaryen #4470 limitation (AsyncifyFlow skips catch bodies) AND the
// fix (the catch-arm-hoisting pre-pass, scripts/binaryen-hoist-pass/).

type Logger = { consoleLogs: string[]; errors: string[] };

const CASES = [
  'throw_across_sleep',
  'fiber_then_throw',
  'suspend_in_catch',
  'transitive_suspend_in_catch',
  'value_returning_catch',
  'suspend_in_catch_on_fiber',
];

const CRASH_SIGNATURES = [
  'function signature mismatch',
  'null function',
  'indirect call to null',
  'unreachable',
  'is not a function',
  'index out of bounds',
];

const all = (l: Logger) => [...l.consoleLogs, ...l.errors];
const passed = (l: Logger, name: string) =>
  l.consoleLogs.some((x) => x.includes(`[EH_SPIKE] PASS ${name}`));
const summary = (l: Logger) => l.consoleLogs.find((x) => x.includes('[EH_SPIKE] SUMMARY'));
const crashLines = (l: Logger) =>
  all(l).filter(
    (x) => CRASH_SIGNATURES.some((s) => x.toLowerCase().includes(s)) && !x.includes('[EH_SPIKE]')
  );

// Shared assertion: the toy boots and all three mechanisms pass cleanly.
async function expectAllGreen(page: any, testLogger: Logger, url: string) {
  await page.goto(url);
  await expect
    .poll(() => summary(testLogger) ?? null, { timeout: 30000, message: 'should emit SUMMARY' })
    .not.toBeNull();
  for (const name of CASES) {
    expect(passed(testLogger, name), `${name} should PASS`).toBe(true);
  }
  expect(crashLines(testLogger), 'no crash signatures').toHaveLength(0);
}

test.describe('wasm-EH spike', () => {
  test('JS-EH baseline: all suspend-in-catch shapes pass', async ({ page, testLogger }) => {
    await expectAllGreen(page, testLogger, '/standalone/eh-spike/eh_spike_jseh.html');
  });

  test('native wasm-EH (no pass): sleep + fiber pass, suspend-in-catch traps (#4470)', async ({
    page,
    testLogger,
  }) => {
    await page.goto('/standalone/eh-spike/eh_spike_wasmeh.html');

    await expect
      .poll(() => passed(testLogger, 'throw_across_sleep'), {
        timeout: 30000,
        message: 'asyncify sleep inside a try + throw should work under wasm-EH',
      })
      .toBe(true);
    expect(passed(testLogger, 'fiber_then_throw'), 'fiber swap + throw should work').toBe(true);

    // The unsupported pattern must fail LOUDLY (trap or 4s watchdog), never silently pass.
    await expect
      .poll(
        () =>
          crashLines(testLogger).length > 0 ||
          all(testLogger).some((x) => x.includes('[EH_SPIKE] FAIL suspend_in_catch')),
        { timeout: 30000, message: 'suspend-in-catch should trap or time out under wasm-EH' }
      )
      .toBe(true);
    expect(
      passed(testLogger, 'suspend_in_catch'),
      'suspend-inside-catch must NOT pass without the hoist pass (Binaryen #4470)'
    ).toBe(false);
  });

  test('native wasm-EH + catch-arm hoist pass: all shapes pass', async ({ page, testLogger }) => {
    await expectAllGreen(page, testLogger, '/standalone/eh-spike/eh_spike_wasmeh_hoist.html');
  });
});
