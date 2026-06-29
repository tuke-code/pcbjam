;; Value-typed try whose catch SUSPENDS: the catch calls an async import and yields its result.
;; Drives the $result routing across a real asyncify unwind/rewind.
(module
 (import "env" "sleep" (func $sleep (param i32) (result i32)))
 (memory (export "memory") 1)
 (tag $cpp (param i32))
 (func $vt (export "vt") (result i32)
  (try (result i32)
   (do (throw $cpp (i32.const 0)))
   (catch $cpp (drop (pop i32)) (call $sleep (i32.const 50))))))
