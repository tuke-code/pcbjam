import { expect, type Page } from '@playwright/test';

/**
 * Open the overlay menu (collab-presence 0010) if its panel isn't already
 * open. The roster, source chip, view-only pill, comment bar and chrome
 * toggle live inside the panel; the FAB (with the peer-count badge) is the
 * only overlay control always on screen. Any canvas click closes the panel
 * (click-away), so specs re-open it before each in-panel interaction.
 */
export async function openOverlayMenu(page: Page): Promise<void> {
  const fab = page.getByTestId('overlay-menu-fab');
  await expect(fab).toBeVisible({ timeout: 30000 });
  if (await page.getByTestId('overlay-menu-panel').count()) return;
  await fab.click();
  await expect(page.getByTestId('overlay-menu-panel')).toBeVisible();
}
