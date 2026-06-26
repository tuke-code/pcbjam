;; Regression repro of SYMBOL_EDIT_FRAME::DuplicateSymbol (KiCad eeschema). A cpp catch arm that is
;; hoisted PAST an ancestor catch_all (the case-6 deferral) carries a nested __cxa_end_catch cleanup
;; (try (do ..) (delegate $M)) whose delegate target $M is a mid try sitting INSIDE that ancestor
;; catch_all. Hoisting the arm out (into the dispatch section, outside every try) orphaned the
;; delegate -> wasm-validator "all delegate targets must be valid, on (delegate $M)". The fix
;; retargets it to DELEGATE_CALLER_TARGET (a delegate can only target a try or the caller, not the
;; $done block); re-throwing a cleanup exception to the caller is the C++ throw-during-cleanup
;; (std::terminate) path, never taken in normal flow. cpp tag = single i32.
(module
 (tag $cpp (param i32))

 ;; Exception path: $A throws cpp(1); its catch_all (re)throws into $inner whose cpp arm sets r:=42,
 ;; its __cxa_end_catch cleanup delegates to the enclosing $M.  expect 42.
 (func $caught (export "caught") (result i32)
  (local $r i32)
  (try $A
   (do
    (throw $cpp (i32.const 1)))
   (catch_all
    (try $M
     (do
      (try $inner
       (do
        (rethrow $A))
       (catch $cpp
        (drop (pop i32))
        (local.set $r (i32.const 42))
        (try
         (do (nop))
         (delegate $M)))
       (catch_all
        (rethrow $A))))
     (catch_all
      (rethrow $A)))))
  (local.get $r))

 ;; No-exception path: $A body falls through with r:=7.  expect 7.
 (func $normal (export "normal") (result i32)
  (local $r i32)
  (try $A
   (do
    (local.set $r (i32.const 7)))
   (catch_all
    (try $M
     (do
      (try $inner
       (do
        (rethrow $A))
       (catch $cpp
        (drop (pop i32))
        (local.set $r (i32.const 42))
        (try
         (do (nop))
         (delegate $M)))
       (catch_all
        (rethrow $A))))
     (catch_all
      (rethrow $A)))))
  (local.get $r))
)
