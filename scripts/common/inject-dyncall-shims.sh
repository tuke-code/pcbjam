#!/bin/bash
# Inject dynCall_* shims into Emscripten-generated JS
#
# Emscripten 4.x no longer generates signature-specific dynCall_* functions,
# but the invoke_* wrappers (for C++ exception handling) still call them.
# This script auto-generates shims using getWasmTableEntry() which IS defined.
#
# Usage: inject-dyncall-shims.sh <pcbnew.js>

set -e

JS_FILE="$1"

if [ -z "$JS_FILE" ] || [ ! -f "$JS_FILE" ]; then
    echo "Error: JS file not found: $JS_FILE"
    echo "Usage: $0 <path/to/pcbnew.js>"
    exit 1
fi

echo "Extracting dynCall signatures from $JS_FILE..."

# Extract all unique dynCall_* signatures from the file
# Matches patterns like: dynCall_i, dynCall_ii, dynCall_viijj, etc.
SIGNATURES=$(grep -oE 'dynCall_[a-zA-Z0-9]+' "$JS_FILE" | sort -u | sed 's/dynCall_//')

if [ -z "$SIGNATURES" ]; then
    echo "No dynCall signatures found - nothing to inject"
    exit 0
fi

SIG_COUNT=$(echo "$SIGNATURES" | wc -l | tr -d ' ')
echo "Found $SIG_COUNT unique signatures"

# Generate shim code
SHIM_FILE=$(mktemp)
cat > "$SHIM_FILE" << 'HEADER'

// === dynCall shims for Emscripten exception handling ===
// Auto-generated: maps dynCall_SIG() calls to getWasmTableEntry()
// This fixes "dynCall_* is not defined" errors in Emscripten 4.x
//
// These shims are asyncify-aware: they track Asyncify.exportCallStack so that
// asyncify can rewind through indirect calls (e.g., main loop callbacks,
// timer callbacks, event handlers). Without this tracking, asyncify's doRewind
// fails because it can't find the entry function to re-enter the WASM module.
HEADER

for sig in $SIGNATURES; do
    # Count args: signature length - 1 (first char is return type)
    argcount=$((${#sig} - 1))

    # Generate argument list: index, a0, a1, a2, ...
    args="index"
    call_args=""
    for ((i=0; i<argcount; i++)); do
        args="$args, a$i"
        if [ $i -gt 0 ]; then
            call_args="$call_args, "
        fi
        call_args="${call_args}a$i"
    done

    cat >> "$SHIM_FILE" << SHIMEOF
function dynCall_$sig($args) {
  var tableFunc = getWasmTableEntry(index);
  if (typeof Asyncify !== 'undefined') {
    var rewindKey = '__dyn_${sig}_' + index;
    if (!wasmExports[rewindKey]) wasmExports[rewindKey] = tableFunc;
    Asyncify.exportCallStack.push(rewindKey);
    try {
      return tableFunc($call_args);
    } finally {
      if (!ABORT) {
        Asyncify.exportCallStack.pop();
        Asyncify.maybeStopUnwind();
      }
    }
  }
  return tableFunc($call_args);
}
SHIMEOF
done

echo "" >> "$SHIM_FILE"
echo "// === End dynCall shims ===" >> "$SHIM_FILE"


# Find the insertion point: after getWasmTableEntry definition
# The pattern is:
#   var getWasmTableEntry = funcPtr => {
#     ...
#   };
# We insert after the closing `};`

# Find line number of getWasmTableEntry definition
GWTL_LINE=$(grep -n '^var getWasmTableEntry = funcPtr => {' "$JS_FILE" | head -1 | cut -d: -f1)

if [ -z "$GWTL_LINE" ]; then
    echo "Warning: Could not find getWasmTableEntry definition"
    echo "Trying alternate pattern..."
    GWTL_LINE=$(grep -n 'var getWasmTableEntry' "$JS_FILE" | head -1 | cut -d: -f1)
fi

if [ -z "$GWTL_LINE" ]; then
    echo "Error: Could not find getWasmTableEntry in $JS_FILE"
    echo "The shims need to be inserted after getWasmTableEntry is defined"
    rm "$SHIM_FILE"
    exit 1
fi

# Find the closing `};` after getWasmTableEntry (within next 10 lines)
INSERT_LINE=""
for ((i=GWTL_LINE; i<=GWTL_LINE+10; i++)); do
    LINE_CONTENT=$(sed -n "${i}p" "$JS_FILE")
    if [[ "$LINE_CONTENT" == "};" ]]; then
        INSERT_LINE=$i
        break
    fi
done

if [ -z "$INSERT_LINE" ]; then
    echo "Warning: Could not find closing }; for getWasmTableEntry"
    echo "Inserting after line $GWTL_LINE"
    INSERT_LINE=$GWTL_LINE
fi

echo "Injecting shims after line $INSERT_LINE..."

# Create output file with shims inserted
head -n "$INSERT_LINE" "$JS_FILE" > "${JS_FILE}.tmp"
cat "$SHIM_FILE" >> "${JS_FILE}.tmp"
tail -n +$((INSERT_LINE + 1)) "$JS_FILE" >> "${JS_FILE}.tmp"

# Replace original file
mv "${JS_FILE}.tmp" "$JS_FILE"
rm "$SHIM_FILE"

echo "Successfully injected $SIG_COUNT dynCall shims into $JS_FILE"

# Stabilize Asyncify rewind targets for Emscripten fibers.
# When a fiber resumes via rewind, the outer JS frame can collapse to the
# innermost dynCall wrapper on the next yield. Giving each fiber its own stable
# rewind wrapper preserves the original re-entry target across later yields.
if grep -q 'Fiber rewind stabilization for Asyncify fibers' "$JS_FILE"; then
    echo "Fiber rewind stabilization already present - skipping"
else
    FIBER_PATCH_FILE=$(mktemp)
    cat > "$FIBER_PATCH_FILE" << 'FIBERPATCH'

// === Fiber rewind stabilization for Asyncify fibers ===
if (typeof Asyncify !== "undefined" && typeof Fibers !== "undefined" && typeof _emscripten_fiber_swap !== "undefined") {
  var __originalAsyncifySetDataRewindFunc = Asyncify.setDataRewindFunc.bind(Asyncify);
  Asyncify.setDataRewindFunc = function(ptr, forcedBottomOfCallStack) {
    if (forcedBottomOfCallStack) {
      var rewindId = Asyncify.getCallStackId(forcedBottomOfCallStack);
      GROWABLE_HEAP_I32()[(((ptr) + (8)) >> 2)] = rewindId;
      return;
    }
    return __originalAsyncifySetDataRewindFunc(ptr);
  };

  Fibers.rewindTargetByFiber ||= {};
  Fibers.rewindWrapperByFiber ||= {};
  Fibers.entryWrapperByFiber ||= {};
  Fibers.entryKeyByFiber ||= {};
  Fibers.shouldUseStableRewindTarget ||= function(fiber) {
    return GROWABLE_HEAP_U32()[(((fiber) + (16)) >> 2)] !== 0;
  };
  Fibers.ensureRewindWrapper ||= function(fiber) {
    var key = "__fiber_rewind_" + fiber;

    if (!Fibers.rewindWrapperByFiber[fiber]) {
      var target = Fibers.rewindTargetByFiber[fiber];

      Fibers.rewindWrapperByFiber[fiber] = function() {
        Asyncify.exportCallStack.push(key);
        try {
          return wasmExports[target]();
        } finally {
          if (!ABORT) {
            Asyncify.exportCallStack.pop();
            Asyncify.maybeStopUnwind();
          }
        }
      };

      wasmExports[key] = Fibers.rewindWrapperByFiber[fiber];
    }

    return key;
  };

  var __originalFinishContextSwitch = Fibers.finishContextSwitch;
  Fibers.finishContextSwitch = function(newFiber) {
    var entryPoint = GROWABLE_HEAP_U32()[(((newFiber) + (12)) >> 2)];

    if (entryPoint !== 0 && !Fibers.entryWrapperByFiber[newFiber]) {
      var userData = GROWABLE_HEAP_U32()[(((newFiber) + (16)) >> 2)];
      var entryKey = "__fiber_entry_" + newFiber;

      Fibers.entryWrapperByFiber[newFiber] = function() {
        return dynCall_vi(entryPoint, userData);
      };
      Fibers.entryKeyByFiber[newFiber] = entryKey;
      wasmExports[entryKey] = Fibers.entryWrapperByFiber[newFiber];
    }

    return __originalFinishContextSwitch(newFiber);
  };

  var __originalEmscriptenFiberSwap = _emscripten_fiber_swap;
  _emscripten_fiber_swap = (oldFiber, newFiber) => {
    if (ABORT) return;

    if (Asyncify.state === Asyncify.State.Normal) {
      Asyncify.state = Asyncify.State.Unwinding;

      var asyncifyData = oldFiber + 20;
      if (Fibers.shouldUseStableRewindTarget(oldFiber)) {
        if (!Fibers.rewindTargetByFiber[oldFiber]) {
          Fibers.rewindTargetByFiber[oldFiber] = Fibers.entryKeyByFiber[oldFiber] || Asyncify.exportCallStack[0];
        }

        var rewindKey = Fibers.ensureRewindWrapper(oldFiber);
        Asyncify.setDataRewindFunc(asyncifyData, rewindKey);
      } else {
        Asyncify.setDataRewindFunc(asyncifyData);
      }
      Asyncify.currData = asyncifyData;
      _asyncify_start_unwind(asyncifyData);

      var stackTop = stackSave();
      GROWABLE_HEAP_U32()[(((oldFiber) + (8)) >> 2)] = stackTop;

      Fibers.nextFiber = newFiber;
      return;
    }

    return __originalEmscriptenFiberSwap(oldFiber, newFiber);
  };

  _emscripten_fiber_swap.isAsync = true;

  // --- handleSleep save/restore for nested Asyncify ---
  // Asyncify.currData is a single-slot global. When a fiber swap runs inside an
  // EM_ASYNC_JS Promise await (e.g., wxDialog::ShowModal via startModal), the
  // fiber swap overwrites currData with the fiber's asyncify_data, losing the
  // sleep's own buffer. On Promise resolution, handleSleep's doRewind then uses
  // the wrong buffer and crashes with "index out of bounds" or "unreachable".
  //
  // Workaround: intercept Asyncify.allocateData to record which pointer belongs
  // to the active handleSleep; restore it to Asyncify.currData inside the wakeUp
  // callback before handleSleep proceeds to _asyncify_start_rewind + doRewind.
  // Documented upstream as Emscripten Issue #9153 (wontfix).
  if (typeof Asyncify.handleSleep === "function"
      && typeof Asyncify.allocateData === "function"
      && !Asyncify.__nestedHandleSleepInstalled) {
    // Stack of handleSleep contexts awaiting their allocateData association.
    // Each context learns its data pointer when allocateData fires.
    Asyncify.__pendingSleepContexts = [];

    var __originalAllocateData = Asyncify.allocateData.bind(Asyncify);
    Asyncify.allocateData = function() {
      var ptr = __originalAllocateData();
      // Associate with the innermost pending handleSleep that hasn't been
      // linked yet. allocateData is called once per sleep's unwind, so the
      // innermost un-linked context is ours.
      for (var i = Asyncify.__pendingSleepContexts.length - 1; i >= 0; --i) {
        var ctx = Asyncify.__pendingSleepContexts[i];
        if (!ctx.capturedData) {
          ctx.capturedData = ptr;
          break;
        }
      }
      return ptr;
    };

    var __originalHandleSleep = Asyncify.handleSleep.bind(Asyncify);
    Asyncify.handleSleep = function(startAsync) {
      var sleepCtx = { capturedData: null, cleanedUp: false };
      Asyncify.__pendingSleepContexts.push(sleepCtx);

      var cleanup = function() {
        if (sleepCtx.cleanedUp) return;
        sleepCtx.cleanedUp = true;
        var idx = Asyncify.__pendingSleepContexts.indexOf(sleepCtx);
        if (idx !== -1) Asyncify.__pendingSleepContexts.splice(idx, 1);
      };

      try {
        return __originalHandleSleep(function(wakeUp) {
          return startAsync(function(result) {
            // wakeUp runs from pure JS on Promise resolution. Fiber swaps
            // during the await may have overwritten Asyncify.currData with
            // a fiber's buffer pointer. Restore OUR buffer so handleSleep's
            // _asyncify_start_rewind and doRewind use the right data.
            if (sleepCtx.capturedData) {
              Asyncify.currData = sleepCtx.capturedData;
            }
            cleanup();
            return wakeUp(result);
          });
        });
      } catch (e) {
        cleanup();
        throw e;
      }
    };

    Asyncify.__nestedHandleSleepInstalled = true;
  }
}
// === End fiber rewind stabilization ===
FIBERPATCH

    FIBER_INSERT_LINE=$(grep -n '^_emscripten_fiber_swap\.isAsync = true;$' "$JS_FILE" | head -1 | cut -d: -f1)

    if [ -z "$FIBER_INSERT_LINE" ]; then
        echo "Error: Could not find _emscripten_fiber_swap.isAsync marker in $JS_FILE"
        rm "$FIBER_PATCH_FILE"
        exit 1
    fi

    echo "Injecting fiber rewind stabilization after line $FIBER_INSERT_LINE..."

    head -n "$FIBER_INSERT_LINE" "$JS_FILE" > "${JS_FILE}.tmp"
    cat "$FIBER_PATCH_FILE" >> "${JS_FILE}.tmp"
    tail -n +$((FIBER_INSERT_LINE + 1)) "$JS_FILE" >> "${JS_FILE}.tmp"

    mv "${JS_FILE}.tmp" "$JS_FILE"
    rm "$FIBER_PATCH_FILE"
fi

# Fix empty callback arrow functions generated by Emscripten with pthreads
# When pthreads is enabled, Emscripten generates empty {} for some direct-call paths
# because it assumes they won't be used. But they ARE used in certain cases.
# The dynCall_* functions exist (generated above), we just need to call them.
echo "Fixing empty callback arrow functions..."

TOTAL_FIXED=0

# Fix 1: HTML5 event callbacks (3 args) - signature iiii
# Pattern: ((a1, a2, a3) => {})(eventTypeId, ...
# Function pointer is 'callbackfunc' in these contexts
COUNT_BEFORE=$(grep -c '((a1, a2, a3) => {})(eventTypeId,' "$JS_FILE" || true)
if [ "$COUNT_BEFORE" -gt 0 ]; then
    sed -i '' 's/((a1, a2, a3) => {})(eventTypeId,/((a1, a2, a3) => dynCall_iiii(callbackfunc, a1, a2, a3))(eventTypeId,/g' "$JS_FILE"
    COUNT_AFTER=$(grep -c '((a1, a2, a3) => {})(eventTypeId,' "$JS_FILE" || true)
    FIXED=$((COUNT_BEFORE - COUNT_AFTER))
    echo "  Fixed $FIXED HTML5 event callback(s) (dynCall_iiii)"
    TOTAL_FIXED=$((TOTAL_FIXED + FIXED))
fi

# Fix 2: invokeEntryPoint pthread entry (1 arg) - signature ii (returns pointer)
# Pattern in invokeEntryPoint: var result = (a1 => {})(arg);
# Function pointer is 'ptr'
COUNT_BEFORE=$(grep -c 'var result = (a1 => {})(arg);' "$JS_FILE" || true)
if [ "$COUNT_BEFORE" -gt 0 ]; then
    sed -i '' 's/var result = (a1 => {})(arg);/var result = dynCall_ii(ptr, arg);/g' "$JS_FILE"
    COUNT_AFTER=$(grep -c 'var result = (a1 => {})(arg);' "$JS_FILE" || true)
    FIXED=$((COUNT_BEFORE - COUNT_AFTER))
    echo "  Fixed $FIXED pthread entry callback(s) (dynCall_ii)"
    TOTAL_FIXED=$((TOTAL_FIXED + FIXED))
fi

# Fix 3: ___call_sighandler (1 arg) - signature vi (void return)
# Pattern: return (a1 => {})(sig);
# Function pointer is 'fp'
COUNT_BEFORE=$(grep -c 'return (a1 => {})(sig);' "$JS_FILE" || true)
if [ "$COUNT_BEFORE" -gt 0 ]; then
    sed -i '' 's/return (a1 => {})(sig);/return dynCall_vi(fp, sig);/g' "$JS_FILE"
    COUNT_AFTER=$(grep -c 'return (a1 => {})(sig);' "$JS_FILE" || true)
    FIXED=$((COUNT_BEFORE - COUNT_AFTER))
    echo "  Fixed $FIXED signal handler callback(s) (dynCall_vi)"
    TOTAL_FIXED=$((TOTAL_FIXED + FIXED))
fi

# Fix 4: _emscripten_async_call timer (1 arg) - signature vi (void return)
# Pattern: var wrapper = () => (a1 => {})(arg);
# Function pointer is 'func'
COUNT_BEFORE=$(grep -c 'var wrapper = () => (a1 => {})(arg);' "$JS_FILE" || true)
if [ "$COUNT_BEFORE" -gt 0 ]; then
    sed -i '' 's/var wrapper = () => (a1 => {})(arg);/var wrapper = () => dynCall_vi(func, arg);/g' "$JS_FILE"
    COUNT_AFTER=$(grep -c 'var wrapper = () => (a1 => {})(arg);' "$JS_FILE" || true)
    FIXED=$((COUNT_BEFORE - COUNT_AFTER))
    echo "  Fixed $FIXED async timer callback(s) (dynCall_vi)"
    TOTAL_FIXED=$((TOTAL_FIXED + FIXED))
fi

# Fix 5: _emscripten_set_main_loop empty iterFunc callback - signature v (void, no args)
# Pattern: var iterFunc = (() => {});
# The 'func' variable is the function pointer passed to _emscripten_set_main_loop
COUNT_BEFORE=$(grep -c 'var iterFunc = (() => {});' "$JS_FILE" || true)
if [ "$COUNT_BEFORE" -gt 0 ]; then
    sed -i '' 's/var iterFunc = (() => {});/var iterFunc = () => dynCall_v(func);/g' "$JS_FILE"
    COUNT_AFTER=$(grep -c 'var iterFunc = (() => {});' "$JS_FILE" || true)
    FIXED=$((COUNT_BEFORE - COUNT_AFTER))
    echo "  Fixed $FIXED main loop callback(s) (dynCall_v)"
    TOTAL_FIXED=$((TOTAL_FIXED + FIXED))
fi

# Fix 6: Asyncify fiber entry callback (1 arg) - signature vi (void return)
# Pattern in Fibers.finishContextSwitch: (a1 => {})(userData);
# Function pointer is 'entryPoint'
COUNT_BEFORE=$(grep -c '(a1 => {})(userData);' "$JS_FILE" || true)
if [ "$COUNT_BEFORE" -gt 0 ]; then
    sed -i '' 's/(a1 => {})(userData);/dynCall_vi(entryPoint, userData);/g' "$JS_FILE"
    COUNT_AFTER=$(grep -c '(a1 => {})(userData);' "$JS_FILE" || true)
    FIXED=$((COUNT_BEFORE - COUNT_AFTER))
    echo "  Fixed $FIXED fiber entry callback(s) (dynCall_vi)"
    TOTAL_FIXED=$((TOTAL_FIXED + FIXED))
fi

if [ "$TOTAL_FIXED" -gt 0 ]; then
    echo "Total: Fixed $TOTAL_FIXED empty callback(s)"
else
    echo "No empty callbacks found - nothing to fix"
fi

# === Diagnostic logging for modal/asyncify/fiber interactions ===
# This instruments key JS functions to trace "index out of bounds" crashes
# that occur after EndModal during Asyncify rewind.
echo "Injecting diagnostic logging for modal/asyncify interactions..."

DIAG_PATCH_FILE=$(mktemp)
cat > "$DIAG_PATCH_FILE" << 'DIAGPATCH'

// === Diagnostic: modal/asyncify/fiber interaction logging ===
(function() {
  var _diagTimerId = 0;
  var _diagModalActive = false;
  var _diagScheduledTimers = {};

  // 1. Instrument _emscripten_async_call to log function pointer validity
  if (typeof _emscripten_async_call !== "undefined") {
    var __diag_orig_async_call = _emscripten_async_call;
    _emscripten_async_call = function(func, arg, millis) {
      var tableSize = wasmTable ? wasmTable.length : -1;
      var inBounds = func < tableSize;
      var id = ++_diagTimerId;
      console.warn('[DIAG_ASYNC_CALL] id=' + id + ' func=' + func + ' arg=' + arg +
                   ' millis=' + millis + ' tableSize=' + tableSize +
                   ' inBounds=' + inBounds + ' modalActive=' + _diagModalActive +
                   ' asyncifyState=' + (typeof Asyncify !== 'undefined' ? Asyncify.state : 'N/A'));
      if (!inBounds) {
        console.error('[DIAG_ASYNC_CALL] ALREADY OUT OF BOUNDS at schedule time! func=' + func);
        console.trace();
      }
      _diagScheduledTimers[id] = { func: func, arg: arg, millis: millis, scheduledDuringModal: _diagModalActive };
      return __diag_orig_async_call(func, arg, millis);
    };
  }

  // 2. Instrument Asyncify.setDataRewindFunc to log rewind target changes
  if (typeof Asyncify !== "undefined" && Asyncify.setDataRewindFunc) {
    var __diag_orig_setRewind = Asyncify.setDataRewindFunc.bind(Asyncify);
    Asyncify.setDataRewindFunc = function(ptr, forcedBottomOfCallStack) {
      console.warn('[DIAG_REWIND_FUNC] ptr=' + ptr + ' forced=' + forcedBottomOfCallStack +
                   ' state=' + Asyncify.state + ' modalActive=' + _diagModalActive +
                   ' callStack=' + JSON.stringify(Asyncify.exportCallStack));
      return __diag_orig_setRewind(ptr, forcedBottomOfCallStack);
    };
  }

  // 3. Track modal lifecycle via _endModal
  //    startModal sets Module._endModal; we wrap that setter to detect modal start/end.
  var __diag_origDefineProperty = Object.defineProperty;
  var __diag_endModalSet = false;
  //    Instead of defineProperty (complex), poll-wrap: patch the endModal assignment
  //    by wrapping startModal's promise resolution. We detect modal start when
  //    _endModal appears on Module, and modal end when it's deleted.
  if (typeof Module !== "undefined") {
    var __diag_checkInterval = setInterval(function() {
      if (Module._endModal && !__diag_endModalSet) {
        __diag_endModalSet = true;
        _diagModalActive = true;
        console.warn('[DIAG_MODAL] Modal started (Module._endModal appeared)' +
                     ' asyncifyState=' + (typeof Asyncify !== 'undefined' ? Asyncify.state : 'N/A'));

        var __diag_origEndModal = Module._endModal;
        Module._endModal = function(code) {
          console.warn('[DIAG_MODAL] EndModal called with code=' + code +
                       ' asyncifyState=' + (typeof Asyncify !== 'undefined' ? Asyncify.state : 'N/A'));
          _diagModalActive = false;
          return __diag_origEndModal(code);
        };
      } else if (!Module._endModal && __diag_endModalSet) {
        __diag_endModalSet = false;
        console.warn('[DIAG_MODAL] Modal cleanup complete (Module._endModal deleted)' +
                     ' asyncifyState=' + (typeof Asyncify !== 'undefined' ? Asyncify.state : 'N/A'));
      }
    }, 50);
  }

  // 4. Instrument dynCall_vi to catch the exact out-of-bounds call
  if (typeof dynCall_vi !== "undefined") {
    var __diag_orig_dynCall_vi = dynCall_vi;
    dynCall_vi = function(funcPtr, arg1) {
      var tableSize = wasmTable ? wasmTable.length : -1;
      if (funcPtr >= tableSize || funcPtr < 0) {
        console.error('[DIAG_DYNCALL_VI] OUT OF BOUNDS! funcPtr=' + funcPtr +
                      ' tableSize=' + tableSize + ' arg1=' + arg1 +
                      ' modalActive=' + _diagModalActive +
                      ' asyncifyState=' + (typeof Asyncify !== 'undefined' ? Asyncify.state : 'N/A'));
        console.trace();
        return;  // Skip the call to prevent crash, let execution continue
      }
      return __diag_orig_dynCall_vi(funcPtr, arg1);
    };
    dynCall_vi.isAsync = true;
  }

  console.warn('[DIAG] Modal/asyncify diagnostic logging installed');
})();
// === End diagnostic logging ===
DIAGPATCH

# Append diagnostic patch at the end of the JS file
cat "$DIAG_PATCH_FILE" >> "$JS_FILE"
rm "$DIAG_PATCH_FILE"
echo "Diagnostic logging injected"
