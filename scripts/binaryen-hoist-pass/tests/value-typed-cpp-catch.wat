;; Hand-written value-typed cpp-catch tries to exercise the $result routing in --hoist-cpp-catches.
;; cpp tag = single i32 param. Each function returns an i32 via a (try (result i32) ...).
(module
 (tag $cpp (param i32))
 ;; exception path: body throws, catch yields 42  -> expect 42
 (func $vt_throw (export "vt_throw") (result i32)
  (try (result i32)
   (do (throw $cpp (i32.const 99)))
   (catch $cpp (drop (pop i32)) (i32.const 42))))
 ;; normal path: body yields 7, catch never runs   -> expect 7
 (func $vt_normal (export "vt_normal") (result i32)
  (try (result i32)
   (do (i32.const 7))
   (catch $cpp (drop (pop i32)) (i32.const 42))))
 ;; payload routing: catch returns the exception payload it caught (123) -> expect 123
 (func $vt_payload (export "vt_payload") (result i32)
  (try (result i32)
   (do (throw $cpp (i32.const 123)))
   (catch $cpp (pop i32))))
)
