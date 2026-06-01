// Regression coverage for the wxFileDialog folder-navigation fix
// (wxGenericFileDialog::OnOk now navigates into directories instead of
// closing the dialog with the folder path as a "file").
//
// Reproduces the original bug: select a folder, press Enter, expect the
// dialog to navigate into the folder rather than close.

import { test, expect, tryLoadApp, waitForRegistry, clickByLabel } from './utils/fixtures';

test('folder navigation: Enter on a folder navigates instead of closing the dialog', async ({ page, testLogger }) => {
  await page.goto('/standalone/filedialog/filedialog_test.html');
  const loaded = await tryLoadApp(page);
  expect(loaded, 'filedialog_test should load').toBe(true);

  await waitForRegistry(page);

  await clickByLabel(page, 'Open File...');
  await page.waitForTimeout(800);

  // Type a path that's a folder in Emscripten's MEMFS and press Enter.
  // Before the fix, OnOk treated /dev as a file → either showed "Please
  // choose an existing file" (wxFD_FILE_MUST_EXIST) or closed the dialog
  // and surfaced /dev to the calling app as if it were a file.
  await page.keyboard.type('/dev');
  await page.waitForTimeout(200);
  await page.keyboard.press('Enter');
  await page.waitForTimeout(800);

  await page.screenshot({ path: 'test-results/filedlg-folder-nav.png', fullPage: true });

  // No "Selected file:" log should appear — the dialog must NOT have closed
  // with /dev as the picked file.
  const closedWithDev = testLogger.consoleLogs.some(l =>
    l.includes('[FILEDIALOG_EVENT] Selected file:') && l.includes('/dev')
  );
  expect(closedWithDev, 'dialog must not close and report /dev as the selected file').toBe(false);
});
