;; Regression repro of PGM_BASE::HandleException (KiCad) — the case-6 shape (a cpp catch nested in an
;; outer try's catch_all cleanup pad) WITH an intervening block in that cleanup that the cpp arm
;; br's to. LLVM emits this for `try{} catch(A&) catch(B&) catch(...)`: the outer try has only a
;; catch_all (destructor cleanup), inside which it (rethrow)s into a nested try whose cpp catch does
;; the __cxa type dispatch and, when done, (br)s OUT to a block ($blk) sitting between the escape
;; target and the arm. Hoisting that arm to the dispatch section orphaned the br — "all break targets
;; must be valid, on (br $blk)". cpp tag = single i32.
(module
 (tag $cpp (param i32))

 ;; Exception path: outer body throws cpp(1); the catch_all reclassifies via (rethrow $outer) inside
 ;; (block $blk); the nested cpp arm handles it (r := 42) then (br $blk) to finish.  expect 42.
 (func $caught (export "caught") (result i32)
  (local $r i32)
  (try $outer
   (do
    (throw $cpp (i32.const 1)))
   (catch_all
    (block $blk
     (try
      (do
       (rethrow $outer))
      (catch $cpp
       (drop (pop i32))
       (local.set $r (i32.const 42))
       (br $blk))
      (catch_all
       (rethrow $outer))))))
  (local.get $r))

 ;; No-exception path: body falls through with r := 7, neither catch runs.  expect 7.
 (func $normal (export "normal") (result i32)
  (local $r i32)
  (try $outer
   (do
    (local.set $r (i32.const 7)))
   (catch_all
    (block $blk
     (try
      (do
       (rethrow $outer))
      (catch $cpp
       (drop (pop i32))
       (local.set $r (i32.const 42))
       (br $blk))
      (catch_all
       (rethrow $outer))))))
  (local.get $r))
)
