// Patch an emscripten glue `<app>.js` so the runtime `ENV` merges `Module.ENV`.
//
// Why: emscripten's generated glue does `var ENV = {};` and (in this toolchain)
// never merges the caller's `Module.ENV`, so setting `Module.ENV` from JS is a
// silent no-op. The standalone's boot.ts uses `Module.ENV.KICAD_TRACE` to drive
// the `?trace=` profiling harness. Crucially, `_environ_get` on a pthread PROXIES
// TO THE MAIN THREAD, so `getenv()` on the app pthread reads the MAIN thread's
// `ENV` via `getEnvStrings()` — which stays empty without this merge. Result:
// `KICAD_TRACE` never reaches `TRACE_MANAGER`, and every `KI_TRACE(...)` is a
// silent no-op. Merging `Module.ENV` makes `?trace=` (and any future Module.ENV
// use) actually work. Safe no-op when `Module.ENV` is unset (normal boots).
//
// This replaces a fragile manual edit that had to be re-applied to the generated
// glue after every build (docs/features/libs/0013). Idempotent; run per app in
// the build's host post-process. Usage: node patch-env-shim.mjs <file.js> [...]

import { readFileSync, writeFileSync } from "node:fs";

// Guard on `typeof Module` (glue-local on main; undefined in some worker realms —
// harmless there, since environ_get proxies to main anyway). Marker keeps it
// idempotent and lets the region-replace below re-normalize a prior insert.
const SHIM =
  'try { if (typeof Module !== "undefined" && Module && Module.ENV)' +
  " for (var _k in Module.ENV) ENV[_k] = Module.ENV[_k]; }" +
  " catch (e) {} /*PCBJAM_ENV_SHIM*/";

// Match `var ENV = {};` plus whatever sits between it and the next glue function
// (`var getExecutableName`), so re-running normalizes an earlier insert instead
// of stacking. The `getExecutableName` anchor is stable across emscripten builds.
const REGION = /var ENV = \{\};[\s\S]*?\n(?=var getExecutableName)/;

let failed = 0;
for (const file of process.argv.slice(2)) {
  let src;
  try {
    src = readFileSync(file, "utf8");
  } catch (e) {
    console.error(`[env-shim] cannot read ${file}: ${e.message}`);
    failed++;
    continue;
  }
  if (!REGION.test(src)) {
    console.error(`[env-shim] no \`var ENV = {};\` anchor in ${file} — skipped`);
    failed++;
    continue;
  }
  const patched = src.replace(REGION, `var ENV = {};\n${SHIM}\n\n`);
  if (patched === src) {
    console.log(`[env-shim] ${file} already current`);
    continue;
  }
  writeFileSync(file, patched);
  console.log(`[env-shim] patched ${file}`);
}

process.exit(failed ? 1 : 0);
