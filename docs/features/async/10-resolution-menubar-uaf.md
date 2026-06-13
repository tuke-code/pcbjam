# 10 — Resolution: the DOM-port regression was a wxMenuBar use-after-free

Status: **root cause found and fixed; confirmed on eeschema** (the 3 deterministic
`eeschema-ui` red cases pass with zero corruption signatures). Full-suite
validation across all apps + cleanup + commit pending.

## TL;DR

The 18 "asyncify-corruption" kicad failures were **not an Asyncify bug at all**.
They were a single **use-after-free of a `wxMenuBar`** that the DOM port leaves
dangling in a frame's child list:

- `wxMenuBar::Attach(frame)` (src/wasm/menu.cpp) lazily calls `Create(frame)` →
  `wxWindowWasm::Create` → `parent->AddChild(this)`, so in the DOM port the menu
  bar is a **real child window** of the frame (it renders as a DOM node and the
  frame's paint walk reaches it). Native ports do NOT do this — their menu bar is
  a separate native object.
- Replacing the menu bar (`wxFrame::SetMenuBar(newBar)` → `DetachMenuBar()` →
  `wxMenuBar::Detach()`) ends in `wxMenuBarBase::Detach()`, which does
  `m_menuBarFrame = NULL; SetParent(NULL);` — it clears `m_parent` but **never
  removes the bar from the frame's `GetChildren()` list.**
- So the old bar sits in the live frame's child list with `m_parent == NULL`.
  When it is later destroyed (KiCad's `ReCreateMenuBar()` deletes the old bar),
  both `~wxWindowWasm` and `~wxWindowBase` gate their `RemoveChild()` on a
  non-NULL parent — so **neither unlinks it**. A freed `wxMenuBar` is now a
  dangling pointer in a live `wxFrame`'s child list.
- Every ~16 ms the parked main loop runs `ProcessEvents → wxApp::Paint →
  DoPaint → PaintChildren`, which calls `child->IsFrozen()`/`IsShown()` —
  **a virtual call on the freed bar** → `call_indirect` through a stale/zeroed
  vtable → `RuntimeError: indirect call to null` (zeroed) or
  `… signature mismatch` (reused). The idle walk (`SendIdleEvents`) is a second
  detonation site for the same dangling pointer.

## The fix (wasm layer only)

`src/wasm/menu.cpp`, `wxMenuBar::Detach()` — unlink the bar from its parent while
it is still alive, before delegating to the base:

```cpp
void wxMenuBar::Detach()
{
    if ( wxWindow* parent = GetParent() )
        parent->RemoveChild(this);   // removes from m_children AND SetParent(NULL)
    wxMenuBarBase::Detach();
}
```

One-file change. KiCad and wx-core stay untouched. (`RemoveChild` already does
`DeleteObject` + `SetParent(NULL)`, so the subsequent base `SetParent(NULL)` is a
no-op and `~wxWindowBase` won't double-unlink.)

## Why this looked like an Asyncify bug for so long

The crash signature (`indirect call to null` / `index out of bounds` /
`signature mismatch`) is identical to the genuine Asyncify family in docs 01–07,
and it appears in the same place (the parked main loop). But:

- the wasm **table is intact** (a slot-flip monitor saw zero non-null→null
  changes) → the null index comes from a **dead C++ object's vtable**, not table
  corruption;
- unwind/rewind bookkeeping, currData, SPs and `exportCallStack` are all clean,
  and the fault fires at `Asyncify.state == Normal` → not a transition race;
- `noExitRuntime = true` → the park throw runs no destructors → not de-park.

The Asyncify machinery is only the **harness** that runs the paint/idle walk; the
corruption is plain C++ object lifetime.

## Why it's DOM-port-only (main/canvas is green)

Only the DOM port makes `wxMenuBar` a real child of the frame (for DOM
rendering). On the canvas/native side the menu bar is not in `GetChildren()`, so
`wxMenuBarBase::Detach()`'s `SetParent(NULL)`-without-`RemoveChild` leaves nothing
dangling. Same wx/kicad commits; the DOM child-ness is the delta.

## Relationship to docs 08 / 09

- **08** correctly excluded the entire Asyncify framing (table intact, clean
  bookkeeping, Normal-state fault) and localized the crash to a virtual call on a
  bad `wxWindow` in the parked-loop tree walk.
- **09** had the **shape** right — a freed window still reachable through a
  parent's `GetChildren()`, detonated by the idle/paint walk — but proposed the
  wrong free-path (destruction-time DOM re-entry) and fix (`~wxWindowWasm`
  reorder + `wx_dom_event` `IsBeingDeleted` guard). That reorder could not help
  here: the menu bar's parent is already `NULL` at destruction (cleared by
  `Detach`), so any destructor-time `RemoveChild` is skipped. The real fix is to
  not create the inconsistency in the first place — unlink on `Detach`.
- The 09 reorder + `wx_dom_event` guard are **reverted** (they did not fix this
  bug and add unrelated divergence). They remain reasonable optional defensive
  hygiene if a future, distinct destruction-time re-entry is ever proven.

## How it was pinned (diagnostic path, for the next hard UAF)

1. `SHIM_DIAGNOSTICS=1` table-integrity monitor → ruled out table corruption.
2. A C++ **live-window set** (`wxWindowBase` ctor insert / dtor erase) checked at
   the tree-walk sites; membership needs no deref, so it flags a freed window in
   any vtable state. The check had to be placed in `PaintChildren` **before** the
   first virtual call on each child (`DoPaint` entry was one virtual call too
   late — the parent's pre-checks crash first).
3. An **occurrence count** of the bad child in the parent's list (`==1`, not a
   duplicate) + the recovered class (`wxMenuBar`) named the bug exactly.

All of these were temporary instrumentation and have been removed; the technique
is recorded here so it can be re-created for the next hard UAF. Two build aids
were used and also removed: a `-g` (keep-names) pass so wasm stack traces are
symbolized, and an override to empty the asyncify removelist. The key signature
to remember: with the wasm table intact, an "indirect call to null" /
"signature mismatch" in the parked-loop paint/idle walk is a **dead C++ object's
vtable**, i.e. an object-lifetime bug — not Asyncify.
