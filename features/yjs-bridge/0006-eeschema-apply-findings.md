# eeschema collab apply — bugfix session findings (2026-06-04)

Investigation + fixes for the four reported eeschema collab apply bugs, plus the
SCH_SHAPE / SCH_SYMBOL converters. Ground truth was gathered with a headless probe
spec and with the **real web app driven in two tabs** (`?collab=1`, demo.kicad_sch,
75 items incl. 26 SCH_SYMBOL) — the only place real GAL painting happens.

## Key correction: apply works headless

The e2e harness's `kicadOpenFile` returns **false**, but the schematic + screens ARE
built, so `SCH_COMMIT::Push` **does take effect** headless (move/delete/add all mutate
the model). The earlier belief that "Push no-ops headless" predated the dyncall-shim
fix. What headless *cannot* do is **paint** (the GL window is incomplete → the
`window.cpp(731) GetScreenPositionOfClientOrigin: no window` assert). So apply logic is
testable headless; rendering must be checked in the real app. The skipped apply test is
now **un-skipped and extended** (move via the `sx/sy/ex/ey` line form, delete, text add,
shape add).

## Bug-by-bug

1. **"Adding text freezes"** — does NOT reproduce through the collab bridge. Verified in
   the real app: a text added locally in tab A (via the `onDelta` emit sink, the exact
   path the native `SCHEMATIC_LISTENER` uses) propagates over BroadcastChannel and tab B
   **applies it and stays responsive**. Single-tab `kicadCollabApply` of a constructed
   `SCH_TEXT` also stays responsive. Conclusion: the user's freeze is the **interactive**
   text placement (`createNewText` → `DIALOG_TEXT_PROPERTIES::ShowQuasiModal`, a modal
   nested event loop — a known Asyncify-wasm hang), not the collab apply/emit path.
   Defensive change anyway: `makeItem` now parents the text to the schematic and applies
   the default text size, mirroring `createNewText`.

2. **"Delete doesn't apply"** — does NOT reproduce. C++ apply-delete works headless
   (`kicadCollabGetPos` → "" after a `removed` delta) AND end-to-end in two tabs (tab B
   drops the item, count decrements). Likely already fixed by the converters + the
   `CallAfter` deferral now in place.

3. **Circles / SCH_SHAPE don't sync** — REAL gap. Emit side added (itemToJson hand-maps
   SHAPE_T + start/end, arc center, bezier control points, stroke width, fill). But
   `added` **reconstruction is DEFERRED** — same wasm trap class as SCH_SYMBOL (below).
   Bisected with EM_ASM markers: `makeItem` + `commit.Add` succeed; the trap is inside
   `SCH_COMMIT::Push`'s **CHT_ADD** path — the GAL `view->Add` of a *new* SCH_SHAPE —
   "memory access out of bounds". The identical `view->Add` succeeds when the shape is
   **loaded from file** and when an existing shape is **moved** (CHT_MODIFY, verified), so
   it's the asyncify `invoke_viii` mis-dispatch of a SCH_SHAPE add-path virtual, only from
   the programmatic CallAfter/apply context. The dyncall shim catches only the
   "signature mismatch" variant; this trap fires inside the wrong function → not safely
   retryable. (`SetParent`/fill-value/layer-warmup were all ruled out by experiment.)

4. **Wire moves only partially converge** — basic two-tab move **does** converge
   (verified: tab A `kicadCollabTestMoveFirst` → tab B matches, count stable, no dup).
   The connected-wire re-split divergence was not deeply reproduced; it remains a known
   connectivity-CRDT limitation (both peers run `RecalculateConnections` on apply).
   Also found: for a `SCH_LINE`, the `changed` path's bare `item->Move()` is a **no-op**
   (line endpoints only move when flagged) — but the emit side always sends
   `sx/sy/ex/ey`, so real wire moves take the working `SetStart/EndPoint` branch.

## SCH_SYMBOL — deferred (wasm parser trap)

The robust approach is the s-expr clipboard blob (0001 §3 mechanism #2): serialize a
single item with `SCH_IO_KICAD_SEXPR::Format(SCH_SELECTION*, …, /*clipboard*/ true)` and
reconstruct with `LoadContent`. Emit works (`itemToSexpr` produced a valid
`(symbol(lib_id …))` blob). **Reconstruction traps**: `LoadContent` → the s-expr parser
hits `RuntimeError: table index is out of bounds` via `invoke_iii` (the exception
trampoline) when called from the apply/`CallAfter` context — even though the *same
parser* works during file load. This is the same Asyncify + indirect-call family as the
original `SCH_ITEM::Move` trap; the dyncall shim only catches `"signature mismatch"`, not
`"table index out of bounds"`, and a C++ `catch(...)` cannot catch a wasm trap (it
escapes and leaves `s_applyingRemote` stuck true). So the blob path was removed. Symbol
**position/move already syncs** via the generic `changed` path; only live symbol-**add**
is unsupported. Next step for symbols: either (a) extend the dyncall/invoke shim to fall
back to `getWasmTableEntry` on `"table index out of bounds"` too, or (b) route the parse
through a tool coroutine, then re-enable the blob converter for SCH_SYMBOL (+ bus
entries, tables, sheets).

## Shared blocker — the asyncify `invoke_*` mis-dispatch from the apply context

Three deferred items (SCH_SHAPE add, SCH_SYMBOL add, and historically `SCH_ITEM::Move`)
all hit the same root cause: a virtual call dispatched through an asyncify-instrumented
`invoke_*` / `dynCall_*` trampoline **mis-dispatches** when invoked from the programmatic
`CallAfter`/apply context (it works from the file-load / main-loop-boot context). The
dyncall shim (`scripts/common/shims/dyncall-binding.js.tmpl`) fixed the **`Move`** case
because that one trapped with `"indirect call signature mismatch"` *at the call boundary*
(safe to fall back to `getWasmTableEntry`). The shape (`invoke_viii` → "memory access out
of bounds") and symbol (`invoke_iii` → "table index is out of bounds") cases trap *inside*
the mis-dispatched function, so a blind retry is unsafe. **Next step for both:** either
extend the shim to catch these at the boundary, or route apply through the TOOL_MANAGER so
it runs inside a tool coroutine (the asyncify root real edits use) — then re-enable the
SCH_SHAPE hand converter and the SCH_SYMBOL s-expr-blob converter.

## Files touched

- `wasm/bindings/eeschema_embind.cpp` — SCH_SHAPE emit fields (itemToJson), defensive
  SCH_TEXT construction (SetParent + default size). SCH_SHAPE/SCH_SYMBOL `added`
  reconstruction documented + deferred. (No kicad-fork change.)
- `tests/kicad/eeschema-collab.spec.ts` — un-skipped + extended the single-page apply
  test (move / delete / text-add).
