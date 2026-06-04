# Init prompt — eeschema collab apply bugfixes (next session)

Paste the block below into a fresh session.

---

We're continuing the **Yjs collaborative-editing** feature on branch `feature/yjs-bridge`
(kicad-wasm, 3 repos: root + kicad + wxwidgets submodules). Milestones 1–2 (pl_editor) are
done, committed, and pl_editor collab works in the web app. Milestone 3 (eeschema) is in
progress: the bridge — native `SCHEMATIC_LISTENER` emit + `SCH_COMMIT` apply, in
`wasm/bindings/eeschema_embind.cpp` with **zero kicad-fork change** — works for read/emit, and
apply works for several item types. A major wasm blocker (programmatic edits trapped with
"indirect call signature mismatch") was fixed in `scripts/common/shims/dyncall-binding.js.tmpl`
(catch the mismatch RuntimeError, fall back to `getWasmTableEntry`).

**First, read these memory entries** (in MEMORY.md): `yjs_bridge_feature`,
`eeschema_collab_asyncify_apply`, `feedback_build_reuse_schematic_cache`,
`wasm_embind_relink_gotcha`, `web_wizard_skip_config_seed`. Also `features/yjs-bridge/0001`,
`0003`.

**Your task: fix these eeschema collab apply bugs** (found testing two browser tabs with
`?collab=1` on a `.kicad_sch` in the web app):
1. **Adding text FREEZES the app** — the SCH_TEXT add path hangs (`doApply` → `makeItem`
   "SCH_TEXT" / `SetText` / `Push`). Highest priority.
2. **Delete doesn't apply at all** — the `removed` → `commit.Remove` path isn't taking effect
   for any type (verify `ResolveItem` + `Remove` + `Push` for deletes).
3. **Circles / graphic shapes (SCH_SHAPE) don't sync** — no converter yet.
4. **Wire moves only partially converge** — likely tab B running its own
   `RecalculateConnections` on apply (re-splitting differently) + missing SCH_SYMBOL/SCH_SHAPE
   converters.
Then if time: add **SCH_SYMBOL** (the big one — needs the lib symbol + fields/orientation) and
**SCH_SHAPE** converters.

**Architecture / key files:**
- C++ bridge: `wasm/bindings/eeschema_embind.cpp` — `doApply`, `itemToJson`, `makeItem`, the
  `COLLAB_LISTENER`, `kicadCollabApply/Snapshot/TestMoveFirst/GetPos`. Apply is deferred via
  `frame->CallAfter(...)`.
- JS reconciler/transport (generic, reused from pl_editor, unchanged):
  `web/apps/frontend/src/wasm/collab/` (reconciler.ts, broadcast-transport.ts, index.ts).
  Collab gated by `?collab=1` in `WasmTool.tsx` (`COLLAB_TOOLS` includes pl_editor + eeschema).

**Build (eeschema, ~8 min):**
`COMPOSE_PROJECT_NAME=kicad-wasm-feature-schematic KICAD_NO_MONITOR=1 ./docker/build.sh eeschema`
(reuse the warm wxWidgets cache — a fresh per-branch container OOMs at exit 137). The build
auto-force-relinks embind-only changes. Then `cd tests && npm run setup:kicad` copies artifacts
to `tests/apps/kicad/` (symlinked into the web app's `public/wasm`). Don't pipe build output;
it logs to `logs/build/`.

**Test / verify:**
- Headless: `cd tests && npx playwright test --config=playwright-kicad.config.ts
  --project=firefox kicad/eeschema-collab.spec.ts`. The snapshot test is green; the apply/two-tab
  tests are `test.skip` because the e2e harness's `kicadOpenFile` returns **false**
  (`OpenProjectFiles` bails before building the connectivity graph in `files-io.cpp` →
  `SCH_COMMIT::Push` no-ops headless). So **apply must be verified in the real web app**, OR fix
  the harness project-load to unblock headless CI (worthwhile).
- Real app: `cd web && pnpm dev` (server :3050 + frontend :3048; `pnpm db:up && pnpm db:migrate`
  once). Open a `.kicad_sch` in eeschema in two tabs with `?collab=1`; devtools console shows
  `[collab]` logs (down/up/BC traffic) and `[collab] no converter for added type X`.

**Fast-debug trick that worked:** patch the built `tests/apps/kicad/eeschema.js` directly (e.g.
add `console.log` to `dynCall_vii` or markers in the apply path) and run the spec WITHOUT a
rebuild; `tests/logs/kicad/eeschema-collab/*.log` captures the browser console. Use granular
`EM_ASM({console.log(...)})` markers in the C++ to bisect a hang/crash.

**Recent commits on the branch:** `e2410bf` (text/label/no-connect converters), `42866c1`
(dynCall fix + wire converters), `11e4db4` (eeschema read/emit). Use `/git-feature-commit` to
commit across the 3 repos (kicad submodule is currently clean — eeschema needs no fork change).
