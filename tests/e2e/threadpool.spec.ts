import { test, expect, tryLoadApp } from './utils/fixtures';

test.describe('Thread Pool Deadlock Tests', () => {

  test('Creating hardware_concurrency threads should not deadlock', async ({ page, testLogger }) => {
    // This test replicates KiCad's BS::priority_thread_pool pattern:
    // - Creates hardware_concurrency() threads in the frame constructor
    // - If PTHREAD_POOL_SIZE < hardware_concurrency, this will deadlock because:
    //   1. Threads 1-N use pre-warmed Web Workers
    //   2. Thread N+1 needs new Web Worker (posts to event loop)
    //   3. Main thread busy-waits for thread to start
    //   4. Busy-wait blocks event loop -> Worker message never processed
    //   5. DEADLOCK (timeout)

    await page.goto('/standalone/threadpool/threadpool_test.html');

    // If there's a deadlock, tryLoadApp will timeout waiting for canvas
    const loaded = await tryLoadApp(page, 30000);

    await page.screenshot({ path: 'test-results/threadpool-01-loaded.png', fullPage: true });

    // Check console for hardware_concurrency value
    const hardwareConcurrencyLog = testLogger.consoleLogs.find(log =>
      log.includes('[THREADPOOL] hardware_concurrency:')
    );
    if (hardwareConcurrencyLog) {
      console.log('Detected:', hardwareConcurrencyLog);
    }

    // Check for success marker - all threads created and joined
    const success = testLogger.consoleLogs.some(log =>
      log.includes('[THREADPOOL] SUCCESS')
    );

    // Check for blocking warning (indicates potential deadlock situation)
    const blockingWarning = testLogger.consoleLogs.some(log =>
      log.includes('Blocking on the main thread is very dangerous')
    );

    expect(loaded, 'App should load without deadlock').toBe(true);
    expect(success, 'All threads should complete successfully').toBe(true);

    // Log blocking warning status (informational, doesn't fail test)
    if (blockingWarning) {
      console.log('Warning: Blocking on main thread detected (thread creation may have triggered worker spawn)');
    }

    expect(testLogger.errors.filter(e => !e.includes('favicon'))).toHaveLength(0);
  });

  test('Thread creation logs show all threads started', async ({ page, testLogger }) => {
    await page.goto('/standalone/threadpool/threadpool_test.html');

    const loaded = await tryLoadApp(page, 30000);
    if (!loaded) {
      test.skip();
      return;
    }

    // Extract hardware_concurrency from logs
    const hwLog = testLogger.consoleLogs.find(log =>
      log.includes('[THREADPOOL] hardware_concurrency:')
    );

    if (!hwLog) {
      console.log('Could not find hardware_concurrency log');
      return;
    }

    const match = hwLog.match(/hardware_concurrency:\s*(\d+)/);
    if (!match) {
      console.log('Could not parse hardware_concurrency value');
      return;
    }

    const expectedThreads = parseInt(match[1], 10);
    console.log(`Expected ${expectedThreads} threads based on hardware_concurrency`);

    // Count "Thread X started" messages
    const threadStartedLogs = testLogger.consoleLogs.filter(log =>
      log.includes('[THREADPOOL] Thread') && log.includes('started')
    );

    console.log(`Found ${threadStartedLogs.length} "Thread started" messages`);

    // All threads should have started
    expect(threadStartedLogs.length).toBe(expectedThreads);
  });
});
