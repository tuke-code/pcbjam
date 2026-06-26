;; The HandleException nested-catchall-exit-block shape, but the nested cpp arm SUSPENDS (calls an
;; async import) before it br's the intervening block. This drives a real Asyncify unwind/rewind
;; through the hoisted arm AND its retargeted br ($blk -> $done), proving the fix doesn't break the
;; suspend/rewind the hoist exists for. Same harness contract as value-typed-suspend.wat: $vt yields 50.
(module
 (import "env" "sleep" (func $sleep (param i32) (result i32)))
 (memory (export "memory") 1)
 (tag $cpp (param i32))
 (func $vt (export "vt") (result i32)
  (local $r i32)
  (try $outer
   (do
    (throw $cpp (i32.const 0)))
   (catch_all
    (block $blk
     (try
      (do
       (rethrow $outer))
      (catch $cpp
       (drop (pop i32))
       (local.set $r (call $sleep (i32.const 50)))
       (br $blk))
      (catch_all
       (rethrow $outer))))))
  (local.get $r))
)
