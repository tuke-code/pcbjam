/*
 * Catch-arm hoisting: a PRE-pass for `--asyncify`.
 *
 * Asyncify cannot suspend from inside a wasm exception-handling `catch` arm — the AsyncifyFlow
 * walker skips catchBodies, because rewind cannot "fall into" a catch handler (Binaryen #4470).
 * KiCad hits this: error dialogs (Asyncify suspensions) opened from C++ catch blocks. See
 * docs/features/wasm-exceptions/05-asyncify-fork-design.md and 07-spike-results-and-opinion.md.
 *
 * This pass OUTLINES C++ catch handlers to ordinary straight-line code after the try, so stock
 * `--asyncify` (run afterwards) instruments them for free. It works per ESCAPE TARGET: a try that
 * is not itself inside another try's catch body (i.e. it sits in instrumentable code). For each
 * escape target it hoists every cpp-tag catch arm in its catch region — the target's own arms AND
 * arms of tries NESTED inside its catch bodies — past the target. The latter is what handles the
 * common shape where the try body has a local with a destructor: LLVM nests the C++ catch inside
 * the cleanup `catch_all` (`catch_all { ~g; try { rethrow } catch $cpp { ... } }`).
 *
 *   becomes, for an escape target $T with cpp arms collected as 1..N:
 *
 *   (block $done
 *     (local.set $flag (i32.const 0))
 *     $T  ;; each cpp arm rewritten to: (local.set $exn_i (pop)) (local.set $flag i) — and it
 *         ;; then COMPLETES (no branch), so control flows out through $T normally
 *     (br_if $done (i32.eqz (local.get $flag)))   ;; no exception → skip handlers
 *     (if (i32.eq $flag 1) HANDLER1')             ;; INLINE handlers — the shape asyncify rewinds
 *     ... )
 *
 * The INLINE shape (catch sets a flag and completes; handler reached by fall-through after the
 * try) is the one stock Asyncify can rewind; a `br` out to a detached dispatch is NOT rewindable.
 * Each HANDLERi' has the payload `pop` replaced by `(local.get $exn_i)` and any `rethrow` of its
 * owning try replaced by `(throw $cpp (local.get $exn_i))`.
 *
 * Scope: cpp tag = a tag with a single i32 param. Escape targets with a concrete result type are
 * skipped (LLVM keeps catch values in locals, so void/unreachable covers the C++ cases). hoist-all
 * by default; HOIST_ONLY_SUSPEND restricts to arms with a directly-reachable suspending import.
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <ir/utils.h>
#include <pass.h>
#include <wasm-builder.h>
#include <wasm.h>

namespace wasm {

namespace {

static bool hoistDebug() {
  static bool on = getenv("HOIST_DEBUG") != nullptr;
  return on;
}

// The C++ exception tag: a tag whose payload is a single i32 (the exception pointer).
static bool isCppTag(Module* module, Name tag) {
  return module->getTag(tag)->params() == Type::i32;
}

// True if a subtree directly calls an import that suspends under Asyncify.
struct SuspendCallFinder : public PostWalker<SuspendCallFinder> {
  Module* module = nullptr;
  bool found = false;
  void visitCall(Call* curr) {
    auto* target = module->getFunction(curr->target);
    if (!target->imported()) {
      return;
    }
    std::string base = target->base.toString();
    if (base.rfind("__asyncjs__", 0) == 0 || base == "emscripten_fiber_swap" ||
        base.rfind("invoke_", 0) == 0) {
      found = true;
    }
  }
};

// Collects (owningTry, armIndex) for every cpp-tag catch arm reachable in the walked subtree.
struct CppArmCollector : public PostWalker<CppArmCollector> {
  Module* module = nullptr;
  std::vector<std::pair<Try*, Index>> sites;
  void visitTry(Try* curr) {
    for (Index i = 0; i < curr->catchTags.size(); i++) {
      if (isCppTag(module, curr->catchTags[i])) {
        sites.push_back({curr, i});
      }
    }
  }
};

// In a hoisted handler: THIS arm's payload pop -> local.get $exn (pops belonging to nested
// catches are left alone); rethrow of the arm's owning try -> throw $cpp (local.get $exn).
struct ArmRewriter : public ExpressionStackWalker<ArmRewriter> {
  Module* module = nullptr;
  Index exnLocal = 0;
  Name owningTry;
  Name cppTag;
  bool insideNestedTry() {
    for (auto* e : expressionStack) {
      if (e->is<Try>()) {
        return true;
      }
    }
    return false;
  }
  void visitPop(Pop* curr) {
    if (insideNestedTry()) {
      return;
    }
    replaceCurrent(Builder(*module).makeLocalGet(exnLocal, Type::i32));
  }
  void visitRethrow(Rethrow* curr) {
    if (curr->target == owningTry) {
      Builder b(*module);
      replaceCurrent(
        b.makeThrow(cppTag, std::vector<Expression*>{b.makeLocalGet(exnLocal, Type::i32)}));
    }
  }
};

// Is a specific Try expression contained in a subtree?
struct TryFinder : public PostWalker<TryFinder> {
  Try* target = nullptr;
  bool found = false;
  void visitTry(Try* curr) {
    if (curr == target) {
      found = true;
    }
  }
};

static bool tryWithin(Try* needle, Expression* haystack) {
  TryFinder f;
  f.target = needle;
  Expression* e = haystack;
  f.walk(e);
  return f.found;
}

} // anonymous namespace

struct HoistCppCatches : public WalkerPass<ExpressionStackWalker<HoistCppCatches>> {
  bool isFunctionParallel() override { return true; }

  std::unique_ptr<Pass> create() override {
    return std::make_unique<HoistCppCatches>();
  }

  int labelCounter = 0;
  bool changed = false;

  // Reusing/mutating Try nodes in place leaves their cached `type` stale (e.g. an arm that
  // returned made the try `unreachable`, but the rewritten arm completes, making it `none`).
  // Asyncify keys off types, so re-finalize the whole function bottom-up after transforming.
  void doWalkFunction(Function* func) {
    changed = false;
    WalkerPass<ExpressionStackWalker<HoistCppCatches>>::doWalkFunction(func);
    if (changed) {
      ReFinalize().walkFunctionInModule(func, getModule());
    }
  }

  // Is the current expression reached through an ancestor try's catch body? Such tries are
  // hoisted past their escape-target ancestor, not on their own.
  bool insideAncestorCatch() {
    for (Index i = 0; i + 1 < expressionStack.size(); i++) {
      auto* anc = expressionStack[i]->dynCast<Try>();
      if (!anc) {
        continue;
      }
      Expression* child = expressionStack[i + 1];
      for (auto* cb : anc->catchBodies) {
        if (cb == child) {
          return true;
        }
      }
    }
    return false;
  }

  void visitTry(Try* curr) {
    if (curr->isDelegate() || insideAncestorCatch()) {
      return;
    }
    // LLVM keeps C++ catch values in locals, so void/unreachable covers the cases; a concrete
    // result type would need routing the body value through a temp local (unhandled).
    if (curr->type != Type::none && curr->type != Type::unreachable) {
      if (hoistDebug()) {
        std::cerr << "[hoist] skip escape-target " << curr->name << ": concrete result type "
                  << curr->type.toString() << "\n";
      }
      return;
    }

    // Collect cpp-catch arms in curr's CATCH region: curr's own arms + arms of nested tries.
    std::vector<std::pair<Try*, Index>> sites;
    for (Index i = 0; i < curr->catchTags.size(); i++) {
      if (isCppTag(getModule(), curr->catchTags[i])) {
        sites.push_back({curr, i});
      }
    }
    for (auto* cb : curr->catchBodies) {
      CppArmCollector collector;
      collector.module = getModule();
      Expression* e = cb;
      collector.walk(e);
      for (auto& s : collector.sites) {
        sites.push_back(s);
      }
    }
    if (sites.empty()) {
      return;
    }

    // Keep only the OUTERMOST cpp arms; a cpp arm nested inside another collected arm is carried
    // along when that arm is hoisted, so it must not be processed separately.
    {
      std::vector<std::pair<Try*, Index>> outer;
      for (auto& s : sites) {
        bool nested = false;
        for (auto& o : sites) {
          if (o.first == s.first && o.second == s.second) {
            continue;
          }
          if (tryWithin(s.first, o.first->catchBodies[o.second])) {
            nested = true;
            break;
          }
        }
        if (!nested) {
          outer.push_back(s);
        }
      }
      sites.swap(outer);
    }

    if (getenv("HOIST_ONLY_SUSPEND")) {
      bool any = false;
      for (auto& [owner, idx] : sites) {
        SuspendCallFinder finder;
        finder.module = getModule();
        finder.walk(owner->catchBodies[idx]);
        if (finder.found) {
          any = true;
          break;
        }
      }
      if (!any) {
        return;
      }
    }

    if (hoistDebug()) {
      std::cerr << "[hoist] escape-target try=" << curr->name << " sites=" << sites.size()
                << " fn=" << getFunction()->name.toString() << "\n";
    }

    Builder builder(*getModule());
    Index flag = Builder::addVar(getFunction(), Type::i32);
    Name doneLabel(std::string("__hoist_done_") + std::to_string(labelCounter++));

    std::vector<Expression*> dispatch;
    bool single = sites.size() == 1;
    int n = 0;
    for (auto& [owner, idx] : sites) {
      ++n;
      Index exnLocal = Builder::addVar(getFunction(), Type::i32);
      Name cppTag = owner->catchTags[idx];
      Expression* handler = owner->catchBodies[idx];

      ArmRewriter rw;
      rw.module = getModule();
      rw.exnLocal = exnLocal;
      rw.owningTry = owner->name;
      rw.cppTag = cppTag;
      rw.walk(handler);

      // The handler must be reachable by FALL-THROUGH for Asyncify's rewind to fast-forward into
      // it. A bare handler (single arm) or a `br_if`-skip guard (multi-arm) works — but NOT an
      // `if (flag==n) …`, because Asyncify wraps the `if` in `if (state==Normal)` and skips its
      // body on rewind, so the suspend inside it is never re-entered.
      if (single) {
        dispatch.push_back(handler);
      } else {
        Name skip(std::string("__hoist_skip_") + std::to_string(labelCounter++));
        dispatch.push_back(builder.makeBlock(
          skip,
          std::vector<Expression*>{
            builder.makeBreak(skip, nullptr,
                              builder.makeBinary(NeInt32,
                                                 builder.makeLocalGet(flag, Type::i32),
                                                 builder.makeConst(Literal(int32_t(n))))),
            handler},
          Type::none));
      }

      // the arm now only captures the payload and sets the flag, then COMPLETES (no branch),
      // so control flows out through the escape target to the inline dispatch.
      owner->catchBodies[idx] = builder.makeBlock(std::vector<Expression*>{
        builder.makeLocalSet(exnLocal, builder.makePop(Type::i32)),
        builder.makeLocalSet(flag, builder.makeConst(Literal(int32_t(n))))});
    }

    // block $done { $flag = 0; <escape target>; br_if $done if $flag == 0; inline handlers }
    std::vector<Expression*> stmts;
    stmts.push_back(builder.makeLocalSet(flag, builder.makeConst(Literal(int32_t(0)))));
    stmts.push_back(curr);
    stmts.push_back(builder.makeBreak(
      doneLabel, nullptr,
      builder.makeUnary(EqZInt32, builder.makeLocalGet(flag, Type::i32))));
    for (auto* d : dispatch) {
      stmts.push_back(d);
    }

    changed = true;
    replaceCurrent(builder.makeBlock(doneLabel, stmts, Type::none));
  }
};

Pass* createHoistCppCatchesPass() { return new HoistCppCatches(); }

} // namespace wasm
