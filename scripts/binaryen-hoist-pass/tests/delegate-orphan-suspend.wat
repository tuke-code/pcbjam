;; The DuplicateSymbol delegate-orphan shape, but the hoisted cpp arm SUSPENDS (calls an async import)
;; before its __cxa_end_catch cleanup (try (do ..) (delegate $M)). Drives a real Asyncify unwind/rewind
;; through the hoisted arm AND its retargeted delegate ($M -> caller), proving the fix doesn't break
;; the suspend/rewind the hoist exists for. Same harness contract as the other -suspend tests: $vt
;; yields 50.
(module
 (import "env" "sleep" (func $sleep (param i32) (result i32)))
 (memory (export "memory") 1)
 (tag $cpp (param i32))
 (func $vt (export "vt") (result i32)
  (local $r i32)
  (try $A
   (do
    (throw $cpp (i32.const 0)))
   (catch_all
    (try $M
     (do
      (try $inner
       (do
        (rethrow $A))
       (catch $cpp
        (drop (pop i32))
        (local.set $r (call $sleep (i32.const 50)))
        (try
         (do (nop))
         (delegate $M)))
       (catch_all
        (rethrow $A))))
     (catch_all
      (rethrow $A)))))
  (local.get $r))
)
