import { test, expect, type Page } from '@playwright/test';
import { openOverlayMenu } from './overlay-menu';

/**
 * Presence roster e2e (collab-presence 0001): two tabs of the real React app on
 * the SAME project file share one collab room over the default BroadcastChannel
 * provider — each tab's PresenceRoster chip must show the OTHER user, and a tab
 * leaving must drop off the peer's roster promptly (pagehide broadcast, not the
 * 30s awareness timeout).
 *
 * Uses pl_editor (demo.kicad_wks): it is in COLLAB_TOOLS and is the smallest
 * wasm, so the two boots stay cheap. Identity comes from the `?user=` slug
 * (the e2e isolation hook config.ts already honors).
 *
 * Provider-agnostic: BC (default checkout) removes instantly via the pagehide
 * broadcast; partykit removes when the server sees the WS close — measured
 * ~5-8s under local wrangler dev (immediate close on real tab close; an
 * about:blank navigation can BFCache the page and stretch this to ~15s, which
 * is why the leave step CLOSES the tab).
 */

const SCOPE = 'default';
const ROUTE = 'demo.kicad_wks';
const TITLE = /demo — Drawing Sheet Editor/i;

async function bootAs(page: Page, user: string): Promise<void> {
  await page.goto(`/${SCOPE}/projects/demo/${ROUTE}?user=${user}`);
  await expect(page.locator('#canvas')).toBeVisible({ timeout: 120000 });
  await expect
    .poll(() => page.title(), {
      message: `${user}: editor never reached the expected title`,
      timeout: 120000,
      intervals: [1000],
    })
    .toMatch(TITLE);
}

test('two tabs on one file see each other in the roster; leaving removes promptly', async ({
  page,
  context,
}) => {
  test.setTimeout(360000); // two full tool boots

  await bootAs(page, 'alice');

  // Before anyone else joins, alice has no roster section in the overlay
  // menu (0010) at all.
  await openOverlayMenu(page);
  await expect(page.getByTestId('presence-roster')).toHaveCount(0);

  const pageB = await context.newPage();
  await bootAs(pageB, 'bob');
  await openOverlayMenu(pageB);

  // Each side shows the other user (avatar keyed by slug), not itself.
  await expect(page.locator('[data-presence-user="bob"]')).toBeVisible({ timeout: 30000 });
  await expect(pageB.locator('[data-presence-user="alice"]')).toBeVisible({ timeout: 30000 });
  await expect(page.locator('[data-presence-user="alice"]')).toHaveCount(0);
  await expect(pageB.locator('[data-presence-user="bob"]')).toHaveCount(0);

  // bob leaves (tab close → BC pagehide broadcast / partykit WS close): alice's
  // roster must clear well inside the 30s awareness timeout.
  await pageB.close();
  await expect(page.getByTestId('presence-roster')).toHaveCount(0, { timeout: 20000 });
});
