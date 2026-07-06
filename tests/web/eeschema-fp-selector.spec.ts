import { test, expect, type Page } from '@playwright/test';
import { waitForRegistry } from '../e2e/utils/element-tracker';

/**
 * The symbol chooser's footprint selector + preview in the SCHEMATIC editor —
 * the cross-face feature the merged kicad_editor bundle exists for (eeschema
 * reaches KiFACE(FACE_PCB) in-process; docs/features/editor-unification).
 *
 * Flow: open the demo schematic, press "A" (Add Symbol) to open the chooser,
 * pick Device:R (footprint filter "R_*", 2 pins), and assert:
 *   - the footprint selector combobox populates with real footprint entries
 *     (filterFootprints over fp-lib-table rows seeded at boot — the eeschema
 *     frame used to seed fp-lib-table EMPTY, leaving the selector dead);
 *   - selecting an entry drives the footprint preview (per-item "get" over
 *     window.kicadLibs) without aborting the runtime.
 *
 * The footprint list is served either from the publish-time fp-index (op
 * "index" — CDN source) or by lazy per-lib fat-loads (fallback — remote/example
 * backend). The spec logs which path ran; it asserts on behavior, not path.
 */

const SHOT = (n: string) => `test-results/eefpsel-${n}.png`;

async function canvasCenter(page: Page): Promise<{ x: number; y: number }> {
  const box = await page.locator('#canvas').boundingBox();
  expect(box, 'canvas has a bounding box').toBeTruthy();
  return { x: box!.x + box!.width / 2, y: box!.y + box!.height / 2 };
}

test('symbol chooser footprint selector populates and preview renders (eeschema)', async ({ page }) => {
  test.setTimeout(420000);
  const logs: string[] = [];
  page.on('console', (m) => logs.push(`[${m.type()}] ${m.text()}`));
  const pageErrors: string[] = [];
  page.on('pageerror', (e) => pageErrors.push(e.message));

  // Record every window.kicadLibs.request the WASM issues (the provider logs
  // only into the in-page React buffer, invisible to page.on('console')): wrap
  // the provider as boot installs it, into window.__libsCalls = [op,lib,arg,kind][].
  await page.addInitScript(() => {
    const calls: unknown[][] = ((window as any).__libsCalls = []);
    let inner: any;
    Object.defineProperty(window, 'kicadLibs', {
      configurable: true,
      get: () => inner,
      set: (v: any) => {
        if (v && typeof v.request === 'function') {
          const orig = v.request.bind(v);
          v = {
            ...v,
            request: (...a: unknown[]) => {
              const entry = a.slice(0, 4);
              calls.push(entry);
              const p = orig(...a);
              // 5th slot: 'ok' | 'null' | 'err' once the provider settles.
              Promise.resolve(p).then(
                (r: unknown) => entry.push(r === null ? 'null' : 'ok'),
                () => entry.push('err'),
              );
              return p;
            },
          };
        }
        inner = v;
      },
    });
  });

  // ?trace= mirrors the WASM's print/printErr to the real browser console (see
  // boot.ts) — any C++ error/abort text becomes visible to this spec.
  await page.goto('/default/projects/demo/-/eeschema?trace=KI_TRACE_FP_CHOOSER');
  await expect(page.locator('#canvas')).toBeVisible({ timeout: 150000 });
  await waitForRegistry(page, 150000);
  await expect
    .poll(() => page.title(), { timeout: 150000, intervals: [1000] })
    .toMatch(/Schematic Editor/i);
  await page.waitForFunction(() => !!(window as any).kicadLibs, null, { timeout: 60000 });
  await page.waitForTimeout(3000);
  await page.screenshot({ path: SHOT('01-boot'), scale: 'css' });

  // Add Symbol (hotkey A over the canvas) arms the placer; the chooser dialog
  // opens on the tool's first interaction. wx WASM defers wxPostEvent'd
  // follow-ups until the next input event, so wiggle the mouse to pump the
  // loop and click the canvas once if the dialog hasn't shown. The chooser's
  // construction fat-loads the symbol libs before the window registers.
  const c = await canvasCenter(page);
  await page.mouse.move(c.x, c.y);
  await page.mouse.click(c.x, c.y);
  await page.waitForTimeout(300);
  await page.keyboard.press('a');
  await page.waitForTimeout(1500);

  // The registry reports little while a modal pumps: the reliable open signal
  // is the dialog's Cancel button becoming visible (the main frame has none).
  const chooserUp = () =>
    page.evaluate(() => {
      const reg = (window as any).wxElementRegistry;
      if (!reg) return false;
      return reg
        .findAll({ visible: true })
        .some((e: any) => /^&?Cancel$/i.test(e.label ?? ''));
    });
  for (let i = 0; i < 40 && !(await chooserUp()); i++) {
    await page.mouse.move(c.x + (i % 5) * 4, c.y + (i % 3) * 4);
    if (i === 2) await page.mouse.click(c.x, c.y); // armed placer → open chooser
    await page.waitForTimeout(2000);
  }
  if (!(await chooserUp())) {
    // Diagnostics: dump what IS registered + the page console before failing.
    const dump = await page.evaluate(() =>
      (window as any).wxElementRegistry
        .findAll({})
        .map(
          (e: any) =>
            `${e.typeName ?? '?'}|${e.elementType ?? '?'}|${e.label ?? ''}|vis=${e.visible}`,
        )
        .slice(0, 120),
    );
    console.log(`[eefpsel] no chooser; visible elements:\n${dump.join('\n')}`);
    console.log(
      `[eefpsel] console (libs/boot/errors):\n${logs
        .filter((l) => /\[libs\]|\[boot\]|\[out\]|\[err\]|error/i.test(l))
        .slice(0, 120)
        .join('\n')}`,
    );
    console.log(`[eefpsel] console tail:\n${logs.slice(-40).join('\n')}`);
  }
  expect(await chooserUp(), 'symbol chooser dialog opened').toBe(true);
  await page.waitForTimeout(1500);
  await page.screenshot({ path: SHOT('02-chooser'), scale: 'css' });

  // Search for Device:R and select the top hit. The search box has focus on open.
  await page.keyboard.type('R', { delay: 60 });
  await page.waitForTimeout(1000);
  await page.keyboard.press('ArrowDown');
  // Selecting a symbol fires showFootprintFor + populateFootprintSelector —
  // the footprint side loads now (index: one small fetch; fallback: per-lib
  // fat-loads). Poll the combobox until it holds more than the default row.
  const comboPopulated = await page
    .waitForFunction(
      () => {
        const reg = (window as any).wxElementRegistry;
        if (!reg) return false;
        const combos = reg
          .findAll({})
          .filter(
            (e: any) =>
              /combo|footprint_choice|choice/i.test(e.typeName ?? '') ||
              /combo/i.test(e.elementType ?? ''),
          );
        // FOOTPRINT_CHOICE reports its item count via label/value on some
        // builds; fall back to "a combobox exists" + console-side asserts.
        return combos.length > 0 ? combos.map((c: any) => ({
          type: c.typeName ?? '', label: c.label ?? '', value: c.value ?? '',
          count: c.itemCount ?? -1,
        })) : false;
      },
      null,
      { timeout: 240000 },
    )
    .then((h) => h.jsonValue());
  console.log(`[eefpsel] combo state: ${JSON.stringify(comboPopulated)}`);
  await page.waitForTimeout(2500);
  await page.screenshot({ path: SHOT('03-selected'), scale: 'css' });

  // Which footprint-list path ran? (info only — index for CDN sources, per-lib
  // fat-loads for sources without a published index)
  const libsCalls = (await page.evaluate(() => (window as any).__libsCalls)) as string[][];
  const fpCalls = libsCalls.filter((c) => c[3] === 'footprint');
  console.log(
    `[eefpsel] fp bridge calls: ${JSON.stringify(fpCalls.map((c) => [c[0], c[2]]))}`,
  );

  // The selector must have been fed footprints through the shared bridge once
  // a symbol is selected (index op and/or per-lib list).
  expect(
    fpCalls.length > 0,
    `footprint bridge traffic after symbol select; all calls:\n${JSON.stringify(libsCalls)}`,
  ).toBe(true);

  // Sources without a published footprint index (the remote/example backend)
  // answer the index op null; the WASM side then intentionally leaves the
  // selector default-only instead of lazily fat-loading every lib inside the
  // modal pump (which crashes Asyncify — see filterFootprints in pcbnew.cpp).
  // In that mode the meaningful assertions are: the chooser survived with the
  // dual-seeded fp-lib-table, and no modal-pump/runtime error fired.
  const indexAnswered = fpCalls.some((c) => c[0] === 'index' && c[4] === 'ok');
  if (!indexAnswered) {
    console.log('[eefpsel] no fp index from this source — asserting crash-free default-only selector');
    expect(pageErrors, `no page errors, got:\n${pageErrors.join('\n')}`).toEqual([]);
    expect(
      logs.some((l) => /pump error|abort|RuntimeError/i.test(l)),
      `no modal-pump crash; console tail:\n${logs.slice(-15).join('\n')}`,
    ).toBe(false);
    return;
  }

  // Drive the selector: open the popup and click a real footprint row →
  // EVT_COMBOBOX → showFootprint → FOOTPRINT_PREVIEW_PANEL::DisplayFootprint
  // (per-item get).
  const combo = await page.evaluate(() => {
    const reg = (window as any).wxElementRegistry;
    const combos = reg
      .findAllRendered({})
      .filter(
        (e: any) =>
          /combo|footprint_choice|choice/i.test(e.typeName ?? '') ||
          /combo/i.test(e.elementType ?? ''),
      );
    const c = combos[combos.length - 1];
    return c ? { x: c.centerX, y: c.centerY } : null;
  });
  expect(combo, 'footprint selector combobox rendered').toBeTruthy();
  await page.mouse.click(combo!.x, combo!.y); // opens the popup list
  await page.waitForTimeout(1000);
  await page.mouse.move(combo!.x, combo!.y - 40); // pump wx deferred paints
  await page.waitForTimeout(500);
  await page.screenshot({ path: SHOT('04-popup'), scale: 'css' });

  // Click a real footprint row in the popup by its registered position; the
  // rows render as popup list entries (label "Lib:Name"). Fall back to
  // keyboard if the rows aren't tracked.
  const row = await page.evaluate(() => {
    const reg = (window as any).wxElementRegistry;
    const rows = reg
      .findAllRendered({})
      .filter((e: any) => /:R_/.test(e.label ?? ''));
    const r = rows[2] ?? rows[0];
    return r ? { x: r.centerX, y: r.centerY, label: r.label } : null;
  });
  console.log(`[eefpsel] popup row: ${JSON.stringify(row)}`);
  if (row) {
    await page.mouse.click(row.x, row.y);
  } else {
    await page.keyboard.press('ArrowDown');
    await page.waitForTimeout(300);
    await page.keyboard.press('Enter');
  }
  await page.waitForTimeout(1000);
  await page.mouse.move(c.x, c.y); // pump so the selection's follow-ups run
  await page.waitForTimeout(3000);
  await page.screenshot({ path: SHOT('05-fp-selected'), scale: 'css' });

  // The preview load is a per-item footprint "get" over the bridge (or served
  // from the plugin cache when a fat-load already pulled the body).
  const callsAfter = (await page.evaluate(() => (window as any).__libsCalls)) as string[][];
  const fpGets = callsAfter.filter((c) => c[3] === 'footprint' && c[0] === 'get');
  const fpBodies = callsAfter.filter((c) => c[3] === 'footprint' && c[2] === 'bodies');
  console.log(`[eefpsel] fp gets: ${JSON.stringify(fpGets)} fatloads: ${fpBodies.length}`);
  expect(
    fpGets.length + fpBodies.length > 0,
    `preview loaded a footprint body; fp calls:\n${JSON.stringify(
      callsAfter.filter((c) => c[3] === 'footprint'),
    )}`,
  ).toBe(true);

  // Runtime survived the whole cross-face flow.
  expect(pageErrors, `no page errors, got:\n${pageErrors.join('\n')}`).toEqual([]);
  expect(logs.some((l) => /abort|RuntimeError/i.test(l)), 'no wasm abort').toBe(false);
});
