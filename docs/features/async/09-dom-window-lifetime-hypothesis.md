# 09 - DOM window lifetime hypothesis

> Goal: explain the current best theory for the DOM-port regression in
> `08-dom-port-regression.md`: where the code goes wrong, why the DOM port
> exposes it, why the Asyncify arbiter is probably not the fix, and what the
> first repair experiment should be.

## Status

Hypothesis, not yet proven by a green run.

The evidence from 08 rules out the old primary suspects: lost
`Asyncify.currData`, a wedged fiber trampoline, a nulled wasm table entry, and
an `ASYNCIFY_REMOVE` instrumentation gap. The remaining failure shape is a wx
window lifetime bug exposed by the DOM-backed control layer.

## Short version

The crash is most likely:

```text
DOM/event/coroutine work destroys or half-destroys a wxWindow
  -> the window remains reachable from a parent GetChildren() list
  -> the next ProcessEvents() idle phase calls SendIdleEvents()
  -> SendIdleEvents() recurses into that stale child pointer
  -> virtual call on dead/invalid wxWindow
  -> wasm call_indirect index 0
  -> RuntimeError: indirect call to null
```

The modal/nested event-loop pump is a trigger because it drives
`ProcessEvents()` while a modal sleep or tool coroutine is parked. It is not
the only trigger: eeschema also crashes on the normal requestAnimationFrame
main-loop tick.

The DOM port likely changed the timing because wx controls now have real DOM
nodes and browser event listeners. Destroying/removing a DOM node can cause
synchronous browser focus/change/input side effects and direct `wx_dom_event`
entries into C++. The old canvas/universal path did not have per-control DOM
nodes doing this during wx object destruction.

## What the crash means

The symbolized eeschema fault is inside:

```text
wxWindowBase::SendIdleEvents(wxIdleEvent&)
  called by wxAppBase::ProcessIdle()
  called by ProcessEvents()
  called by the rAF main-loop tick
```

`SendIdleEvents()` does these indirect calls per window:

```text
this->OnInternalIdle()
this->HandleWindowEvent(event)
child->SendIdleEvents(event)
```

In wasm, "indirect call to null" with an intact function table means the call
target index is 0. In this code path that is not a JS table corruption story;
it means one of the C++ objects being used for a virtual call is bad.

The current temp diagnostic in `wxwidgets/src/common/wincmn.cpp` checks exactly
that: before recursing into each child during the idle walk, it validates the
child pointer and vtable pointer. If it logs a bad child, we have the parent
window, class, and name that held the stale pointer.

## Why this is not primarily Asyncify

The old async bugs from docs 01-07 were real, but their signatures were
different:

- wrong sleep buffer or `currData` loss -> bad `doRewind()` buffer,
  often `index out of bounds`;
- trampoline guard stuck -> later fiber swaps never resume;
- pump rejection -> modal/nested loop silently remains parked.

The 08 traces show:

- unwind/rewind bookkeeping alternates cleanly;
- stack pointers match the fiber saved-SP slot;
- `exportCallStack` is rebuilt correctly by rewind replay;
- `Asyncify.state` is Normal at the idle crash;
- the wasm function table does not change and no non-null slot flips to null;
- gerbview can throw synchronously inside `ProcessEvents()` with no intervening
  sleep/fiber swap during that `ccall`.

An Asyncify arbiter would make sleep/fiber buffer ownership cleaner, and it is
still the right blueprint if we see wakeups during active transitions. But it
does not stop wx from walking a dangling window pointer during idle.

## Why main can work with the same pump

Root `main` points wxwidgets at the async-hardening commit that already has the
modal/nested pump fixes:

```text
startModal()
  -> setTimeout pump
  -> await ccall("ProcessEvents", ..., { async: true })

wxWasmRunNestedLoop()
  -> same pump shape for nested/quasi-modal loops
```

The DOM-port branch did not introduce those pumps. The major new ingredient is
the DOM-backed controls layer:

- `build/wasm/wx-dom.js` creates real `<input>`, `<select>`, `<button>`,
  fieldset, toolbar, notebook tab, and menu elements.
- `src/wasm/domevents.cpp` routes DOM event listeners directly into
  `wxWindowWasm::OnDomEvent()`.
- `src/wasm/window.cpp` now destroys and updates DOM nodes from wx window
  methods and destructors.

So "main works" should be read as "the canvas port does not hit this DOM
window-lifetime edge." It is not proof that the modal pump itself is harmless
in every port, but it does mean reverting the pump is unlikely to be the real
fix.

## The suspect ordering

The highest-risk ordering is currently in `wxWindowWasm::~wxWindowWasm()`:

```cpp
wxWindowWasm::~wxWindowWasm()
{
    if (m_domId)
    {
        wxDomUnregisterWindow(m_domId);
        wxDomDestroyControl(m_domId);   // JS side effect: element.remove()
        m_domId = 0;
    }

    UnregisterElement(this);            // JS crossing

    SendDestroyEvent();                 // only now m_isBeingDeleted=true

    ...

    DestroyChildren();
}
```

The risky part is that DOM removal and registry JS calls happen before the wx
object is marked as being deleted. Browser DOM removal can synchronously affect
focus and can cause event listeners or document-level forwarding to run. If
that re-enters wx before the object is marked deleting and before it has been
detached from its parent, another `ProcessEvents()`/idle pass can still find
the object through `GetChildren()`.

`wxWindowBase::~wxWindowBase()` eventually removes the object from its parent:

```cpp
if (m_parent)
    m_parent->RemoveChild(this);
```

But that happens after the derived `wxWindowWasm` destructor body has already
run. For a DOM-backed port, that may be too late.

## First fix experiment

Try making wx lifetime state true before any JS/DOM side effects.

Conceptual patch:

```cpp
wxWindowWasm::~wxWindowWasm()
{
    // Mark this object deleting while the most-derived object is still intact.
    SendDestroyEvent();

    // Make re-entrant idle/tree walks unable to find this window through
    // the parent child list before any DOM removal can fire browser side
    // effects.
    if (GetParent())
        GetParent()->RemoveChild(this);

    if (m_domId)
    {
        wxDomUnregisterWindow(m_domId);
        wxDomDestroyControl(m_domId);
        m_domId = 0;
    }

    UnregisterElement(this);

    ...

    DestroyChildren();
}
```

And harden DOM event dispatch:

```cpp
void wx_dom_event(int domId, int kind)
{
    ...
    wxWindowWasm *window = it->second;

    if (window->IsBeingDeleted() || !window->IsEnabled())
        return;

    window->OnDomEvent(static_cast<wxDomEventKind>(kind));
}
```

That is the narrowest wasm-layer fix to test. It keeps KiCad and wx core close
to upstream, and it does not remove modal pumping.

## Cautions for the patch

This needs a real run, not just reasoning.

Things to check while implementing:

- `SendDestroyEvent()` is already idempotent via `m_isBeingDeleted`, so calling
  it earlier should be okay.
- `wxWindowBase::~wxWindowBase()` will later see `m_parent == NULL` if we
  detached early, so it will not remove twice.
- `DestroyChildren()` should still work because children remain in this
  window's own child list; only this window is detached from its parent.
- Some wx code may expect the parent pointer during destroy events. Calling
  `SendDestroyEvent()` before detach preserves that.
- DOM event dispatch should ignore deleting windows even if a stale DOM event
  arrives after `wxDomUnregisterWindow()` missed it or was racing with JS
  listener execution.

## Validation plan

1. Keep the `wincmn.cpp` idle-walk diagnostic for one run and confirm whether
   it logs a bad child. The log should name the parent that still held the
   stale child.

2. Apply the destructor-ordering patch plus `IsBeingDeleted()` guard in
   `wx_dom_event()`.

3. Rebuild KiCad with the normal docker build:

```sh
./docker/build.sh
```

4. Run the deterministic red case from the tests directory:

```sh
cd tests
npm run test:kicad
```

For a faster targeted run, use the same single-spec reproduction from 08 if
the local test script supports it, but final confidence needs the normal
`npm run test:kicad` path.

5. Inspect screenshots and logs:

```text
tests/logs/kicad/<test-name>/
```

The success criteria are:

- no `RuntimeError: indirect call to null`;
- no `index out of bounds`;
- no `signature mismatch`;
- no `.errors.log`;
- no new visual regression in screenshots.

## If that fails

If early detach does not fix it, the next likely culprits are still wx/DOM
lifetime, not the JS Asyncify arbiter:

- a duplicate child-list entry, where `RemoveChild()` removes one node and a
  second copy remains dangling;
- re-entrant `ProcessEvents()` running `ProcessIdle()` or
  `DeletePendingObjects()` while an outer `ProcessEvents()` is still processing
  events/paint/idle;
- `UpdateDomGeometryRecursive()`, `UpdateDomVisibility()`, or `PaintChildren()`
  iterating a child list that mutates underneath it;
- focus teardown, especially `wxDomFocus()`, `focusout`, and DOM element
  removal during `KillFocus()`/destruction.

Those would call for a small wasm event-loop reentrancy guard, or child-list
iteration hardening in the wasm layer, but only after the destructor-ordering
experiment has a red/green result.

## Relationship to the arbiter question

The Design A arbiter remains useful if future logs show a real
sleep/fiber-transition race:

```text
handleSleep entered while Asyncify.state == Rewinding
invalid state abort
doRewind using another context's buffer
stuck trampoline guard without the current self-heal
```

The current 08 signature is different. It is a normal-state virtual call on a
bad wx object reached from idle traversal. An arbiter would not remove that bad
object from `GetChildren()`, and reverting modal pumping would only hide some
of the ways `ProcessEvents()` reaches the bad list.

