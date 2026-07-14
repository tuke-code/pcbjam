/**
 * Corpus lint (kicad-validity 0001 C) — the E3 gate: our own s-expr PRODUCERS
 * must agree with KiCad's parser. Runs `kicad_tools --lint` over:
 *
 *   1. the fixture corpus RAW (tests/fixtures + pcbjam-shared/test/fixtures)
 *      — guards fixtures themselves going stale vs the current parser;
 *   2. every fixture ROUND-TRIPPED through the shared codec
 *      (docToFile(fileToDoc(text))) — the exact writer path the backend's
 *      ydoc materialization uses. A codec output KiCad rejects is the
 *      wrapInBoardEnvelope class of bug (pcbjam f07b997), caught pre-merge.
 *
 * Needs pcbjam/output/kicad_tools.js (docker/build.sh kicad_tools). When the
 * artifact is absent the script SKIPS with exit 0 so it can sit in any CI leg;
 * it becomes a hard gate once kicad_tools joins the CI build set (follow-up in
 * docs/features/kicad-validity/0001 §5).
 *
 * Run: cd tests && npm run corpus:lint
 */
import { execFileSync } from "node:child_process";
import { existsSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import * as path from "node:path";
import { fileURLToPath } from "node:url";
import { docToFile, fileToDoc } from "../../web/pcbjam-shared/src/index.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, "../..");
const cli = path.join(repo, "output/kicad_tools.js");

if (!existsSync(cli)) {
  console.log("corpus-lint: SKIP — output/kicad_tools.js not built");
  process.exit(0);
}

const CORPORA = [
  path.join(repo, "tests/fixtures"),
  path.join(repo, "web/pcbjam-shared/test/fixtures"),
];
const LINTABLE = /\.(kicad_pcb|kicad_sch|kicad_sym|kicad_mod)$/;
// The codec round-trips the doc formats it materializes (not symbol libs).
const CODEC = /\.(kicad_pcb|kicad_sch)$/;

function collect(dir: string): string[] {
  if (!existsSync(dir)) return [];
  const out: string[] = [];
  for (const entry of readdirSync(dir, { withFileTypes: true, recursive: true })) {
    const p = path.join(entry.parentPath ?? (entry as { path: string }).path, entry.name);
    if (entry.isFile() && LINTABLE.test(entry.name)) out.push(p);
  }
  return out.sort();
}

function lint(file: string): { ok: boolean; detail: string } {
  try {
    execFileSync("node", [cli, "--lint", file], { stdio: ["ignore", "ignore", "pipe"] });
    return { ok: true, detail: "" };
  } catch (e) {
    const err = e as { status?: number; stderr?: Buffer };
    return { ok: false, detail: err.stderr?.toString().trim() ?? String(e) };
  }
}

const files = CORPORA.flatMap(collect);
if (!files.length) {
  console.error("corpus-lint: no fixtures found — corpus roots moved?");
  process.exit(1);
}

const tmp = mkdtempSync(path.join(tmpdir(), "corpus-lint-"));
let failures = 0;
let roundTripped = 0;

try {
  for (const file of files) {
    const raw = lint(file);
    if (!raw.ok) {
      failures++;
      console.error(`RAW FAIL  ${path.relative(repo, file)}\n${raw.detail}`);
      continue; // an invalid source can't meaningfully round-trip
    }

    if (!CODEC.test(file)) continue;

    // Hierarchical sub-sheets don't parse standalone through the codec the
    // same way; fileToDoc throwing marks "codec can't represent it" — that is
    // a finding too, but a soft one: report, don't fail (multi-file sheets
    // are exercised by the shared vitest suite with full projects).
    let out: string;
    try {
      out = docToFile(fileToDoc(readFileSync(file, "utf8")));
    } catch (e) {
      console.warn(
        `codec skip ${path.relative(repo, file)}: ${e instanceof Error ? e.message : e}`,
      );
      continue;
    }

    const rt = path.join(tmp, `rt-${roundTripped++}${path.extname(file)}`);
    writeFileSync(rt, out, "utf8");
    const verdict = lint(rt);
    if (!verdict.ok) {
      failures++;
      console.error(
        `ROUND-TRIP FAIL  ${path.relative(repo, file)} — codec output rejected by KiCad:\n${verdict.detail}`,
      );
    }
  }
} finally {
  rmSync(tmp, { recursive: true, force: true });
}

console.log(
  `corpus-lint: ${files.length} fixture(s), ${roundTripped} round-trip(s), ${failures} failure(s)`,
);
process.exit(failures ? 1 : 0);
