import { test as base } from '@playwright/test';
import { setupTestLogger, writeTestLogs, TestLogger, MAIN_CANVAS, waitForApp, tryLoadApp, getCanvasBox } from './test-utils';

// Extend base test with automatic logging
export const test = base.extend<{
  testLogger: TestLogger;
}>({
  testLogger: async ({ page }, use, testInfo) => {
    // Build test name from describe block + test title
    const testName = testInfo.titlePath.join(' - ');

    const logger = setupTestLogger(page);

    await use(logger);

    // Write logs after test completes
    writeTestLogs(testName, logger);
    logger.cleanup();
  },
});

export { expect } from '@playwright/test';
export { MAIN_CANVAS, waitForApp, tryLoadApp, getCanvasBox };
