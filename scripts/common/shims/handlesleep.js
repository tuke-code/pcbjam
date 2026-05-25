// === Nested-Asyncify handleSleep currData save/restore (Emscripten #9153) ===
//
// Asyncify.currData is a single-slot global. When a fiber swap runs inside an
// EM_ASYNC_JS Promise await (e.g., wxDialog::ShowModal via startModal), the
// fiber swap overwrites currData with the fiber's asyncify_data, losing the
// sleep's own buffer. On Promise resolution, handleSleep's doRewind then uses
// the wrong buffer and crashes with "index out of bounds" or "unreachable".
//
// Workaround: intercept Asyncify.allocateData to record which pointer belongs to
// the active handleSleep; restore it to Asyncify.currData inside the wakeUp
// callback before handleSleep proceeds to _asyncify_start_rewind + doRewind.
if (typeof Asyncify !== "undefined") {
  if (typeof Asyncify.handleSleep === "function"
      && typeof Asyncify.allocateData === "function"
      && !Asyncify.__nestedHandleSleepInstalled) {
    // Stack of handleSleep contexts awaiting their allocateData association.
    Asyncify.__pendingSleepContexts = [];

    var __originalAllocateData = Asyncify.allocateData.bind(Asyncify);
    Asyncify.allocateData = function() {
      var ptr = __originalAllocateData();
      // Associate with the innermost pending handleSleep not yet linked.
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
            // wakeUp runs from pure JS on Promise resolution. Fiber swaps during
            // the await may have overwritten Asyncify.currData. Restore OUR buffer
            // so handleSleep's _asyncify_start_rewind and doRewind use it.
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
// === End nested-Asyncify handleSleep fix ===
