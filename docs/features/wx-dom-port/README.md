# wxWidgets DOM port (`feature/wx-dom-port`)

THE wxWidgets WASM port: widgets are **real HTML elements**
(`wxButton` → `<button>`, `wxTextCtrl` → `<input>`, menus → popup divs)
instead of pixels on a canvas. Goals: styleable modern UI, native text
input/IME, accessibility, crisp rendering.

Status: COMPLETE and consolidated (2026-06-12). The original canvas
(wxUniversal) mode and all dual-build plumbing have been **removed** —
`--with-wasm` builds the DOM port, full stop. All `src/univ/` +
`include/wx/univ/` modifications were reverted to upstream v3.2.6, so the
wxwidgets fork is upstream + new `src/wasm/` + `include/wx/wasm/` +
`build/wasm/` files + thin hooks. All six KiCad apps (pcbnew, eeschema,
calculator, pl_editor, symbol_editor, gerbview) build and run on it.

Build: `docker/build.sh <app>` (outputs in `output/`), wx standalone via
`scripts/build-wx-wasm.sh`, test apps via `scripts/build-wasm-test.sh`.
Test: `npm test` / `npm run test:kicad` from `tests/` (no env vars).
Work intentionally stays on `feature/wx-dom-port` (root + wxwidgets +
kicad) — not merged to `main`/`wasm-port`.

## Architecture in one page

- **C++ owns state; the DOM is a projection.** Controls call
  `WasmCreateDomNode("type")` once in `Create()`; the shared
  `wxWindowWasm` machinery then syncs geometry (TLW-relative absolute
  positioning, recursing into descendants; ancestor-viewport clipping via
  `clip-path`), visibility (`IsShownOnScreen`, whole subtree),
  enabled/font/focus/destruction. DOM-native state (typed text, checked,
  selection) syncs back through events into C++ caches so getters stay
  synchronous. NB: `GetScreenPosition()` is the CLIENT-AREA origin
  (`ClientToScreen(0,0)`), not the top-left — the projection subtracts
  `GetClientAreaOrigin()` (matters for wxNotebook's tab strip).
- **Layout is wx sizers, not CSS.** DOM contributes intrinsic measurement
  only (clone-based, in an offscreen always-rendered host — sizers run
  before `Show()`); positions are written as absolute `left/top/w/h`.
- **Events** flow element listener → `ccall('wx_dom_event')` →
  `domevents.cpp` routing table → virtual `wxWindowWasm::OnDomEvent` —
  the same direct-dispatch pattern as the port's mouse callbacks (proven
  against Asyncify-suspended modals). Document-level listeners forward
  mouse activity over DOM children into the wx hit-test pipeline
  (`wx_dom_mouse`); wheel events walk up the window hierarchy. Keyboard
  arbitration: keystrokes go untouched to focused DOM editables (Escape
  excepted).
- **Canvas islands.** Owner-drawn/generic widgets (wxGrid, listctrl,
  tree, AUI, STC, calendar...) keep painting via the shared `dc.cpp`
  Canvas2D path inside per-window canvases — the same architecture native
  ports use for owner-drawn widgets.
- **Menus/toolbars** serialize C++→JS as JSON; popups are DOM divs;
  command ids route back via `wxDOM_EVENT_MENU`/`TOOL`. The DOM-native
  wxNotebook renders a real tab strip (`<button role=tab>`).
- **E2E registry.** `window.wxElementRegistry` (wx.js) carries element
  geometry for Playwright. wxWindow-level entries come from
  `src/wasm/window.cpp`; canvas-island content (grid cells, list rows,
  calendar dates, AUI parts...) is published from thin paint-site hooks
  that call helpers in `src/wasm/elementtracker.cpp`; DOM-native
  composites (tabs, menu items, tools, spin arrows, text fields) are
  mirrored by wx-dom.js. One contract for everything.

Key files: `wxwidgets/build/wasm/wx-dom.js` (the whole JS control layer),
`wxwidgets/include/wx/wasm/private/dom.h` (C++→JS bridge),
`wxwidgets/src/wasm/domevents.cpp` (event routing + bitmap data URLs),
`wxwidgets/src/wasm/window.cpp` (DOM-backing machinery),
`wxwidgets/src/wasm/elementtracker.cpp` (e2e registry bridge).

## What works (e2e-verified)

Full `tests/e2e` suite green; full kicad suite green.

- Native DOM: stattext, button (stock labels, default size), textctrl
  (single/multi/password, two-way sync, wxEVT_TEXT/_ENTER), checkbox,
  radiobutton (HTML name groups from wxRB_GROUP chains), radiobox,
  togglebutton, gauge, slider, statline, statbox, statbmp (PNG data
  URLs), bitmap buttons, choice, listbox, editable combobox
  (input+datalist), checklistbox, spinbutton (drives generic wxSpinCtrl),
  notebook (DOM tab strip), menubar+menus, toolbar, tooltips (#wx-tooltip
  layer, island widgets included), dialogs (shared Asyncify ShowModal),
  frame bar geometry.
- Canvas islands verified visually: AUI, calendar, virtual listctrl,
  grid, tree, propgrid, STC.

## Known gaps / polish (tracked in visual-notes.md)

- wxSpinCtrl text field collapses beside the spin pair (generic composite
  sizing); bitmap-button vertical centering; checklistbox selection
  highlight; wxLB_SINGLE uses a multiple `<select>`; native scrollbars
  replace univ gutters.
- Element-registry hooks remain in `src/generic/` + aui/stc/propgrid as
  thin guarded one-liners (canvas islands have no per-item DOM, so tests
  need C++-fed geometry); everything heavier lives in
  `src/wasm/elementtracker.cpp`.

## Fork surface (vs upstream wxWidgets v3.2.6)

Upstream + new files (`src/wasm/`, `include/wx/wasm/`, `build/wasm/`),
the `__WXWASM__` dispatch branches in `include/wx/*.h` (2–4 lines each),
WASM toolkit registration in the build system (bakefile-generated),
wasm-generic fixes in `src/common/`, a handful of unguarded island
rendering tweaks in `src/generic/` (renderg/gridctrl/stattextg/msgdlgg/
spinctlg/filedlgg), and thin `#ifdef __EMSCRIPTEN__` tracker shims at
paint sites. `src/univ/`, `include/wx/univ/`, `src/generic/notebook.cpp`,
`tabg.cpp`, `build/msw/`, `tests/makefile.vc` are byte-identical to
upstream.

See `visual-notes.md` for the full bug log the cross-port screenshot
comparison protocol produced.
