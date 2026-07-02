/**
 * Generate tests/screenshot-manifest.json — the canonical list of expected
 * screenshots + the engine that renders each.
 *
 * The NAME list is authoritative (it's the committed baseline set) and is what
 * lets compare/promote tell an intentional REMOVAL from a flaky/absent render.
 * The ENGINE tag is best-effort (attributed by scanning which spec writes each
 * `test-results/<prefix>` and which project runs that spec) and only feeds the
 * per-engine floors — refine after calibration.
 *
 * Engine routing (from the two playwright configs):
 *   e2e/*.spec.ts               → chromium-swiftshader   (npm run test — wx suite)
 *   kicad/*.spec.ts             → firefox-llvmpipe, or chromium-swiftshader if the
 *                                 spec is in PCBNEW_FAMILY_SPECS (chromium-ci on CI)
 *   web/*.spec.ts               → firefox-llvmpipe        (web config, --project=firefox)
 *
 * CLI (from tests/):  tsx tools/screenshots/gen-manifest.ts [--check]
 *   --check exits 1 if the committed manifest is stale (for CI hygiene).
 */
import * as fs from 'fs';
import * as path from 'path';
import { BASELINE_DIRS, MANIFEST_PATH, isIgnored, type Manifest } from './config';

const CHROMIUM = 'chromium-swiftshader';
const FIREFOX = 'firefox-llvmpipe';
const DEFAULT_ENGINE = CHROMIUM; // baseline-screenshots is dominated by the wx suite

function listSpecs(dir: string): string[] {
    const out: string[] = [];
    if (!fs.existsSync(dir)) return out;
    for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
        const p = path.join(dir, e.name);
        if (e.isDirectory()) out.push(...listSpecs(p));
        // Only spec files — a screenshot literal in a util (e.g. completeWizard's
        // wizard-*) would be attributed to the util's dir, not its real caller.
        // Leaving those unmatched lets them fall to the correct chromium default.
        else if (e.name.endsWith('.spec.ts')) out.push(p);
    }
    return out;
}

/** pcbnew-family spec basenames (routed to chromium-ci on CI), read from the config. */
function pcbnewFamily(root: string): Set<string> {
    const cfg = fs.readFileSync(path.join(root, 'playwright-kicad.config.ts'), 'utf8');
    const block = cfg.match(/PCBNEW_FAMILY_SPECS\s*=\s*\[([\s\S]*?)\]/)?.[1] ?? '';
    return new Set([...block.matchAll(/'[^']*?([\w.-]+\.spec\.ts)'/g)].map((m) => m[1]));
}

function engineForSpec(root: string, specPath: string, family: Set<string>): string {
    const rel = path.relative(root, specPath);
    const base = path.basename(specPath);
    if (rel.startsWith('e2e/')) return CHROMIUM;
    if (rel.startsWith('web/')) return FIREFOX;
    if (rel.startsWith('kicad/')) return family.has(base) ? CHROMIUM : FIREFOX;
    return DEFAULT_ENGINE;
}

/** Build prefix → engine from every `test-results/<prefix>` literal in the specs. */
function prefixEngineMap(root: string, family: Set<string>): Array<{ prefix: string; engine: string }> {
    const map = new Map<string, string>();
    for (const dir of ['e2e', 'kicad', 'web']) {
        for (const spec of listSpecs(path.join(root, dir))) {
            const engine = engineForSpec(root, spec, family);
            const content = fs.readFileSync(spec, 'utf8');
            for (const m of content.matchAll(/test-results\/([A-Za-z0-9_-]+)/g)) {
                const prefix = m[1];
                // First writer wins; a chromium spec shouldn't be overridden by a later firefox one for the same literal.
                if (!map.has(prefix)) map.set(prefix, engine);
            }
        }
    }
    // Longest prefix first so the most specific match wins.
    return [...map.entries()].map(([prefix, engine]) => ({ prefix, engine })).sort((a, b) => b.prefix.length - a.prefix.length);
}

function listBaselines(root: string): string[] {
    const names = new Set<string>();
    for (const dir of BASELINE_DIRS) {
        const abs = path.join(root, dir);
        if (!fs.existsSync(abs)) continue;
        for (const f of fs.readdirSync(abs)) if (f.toLowerCase().endsWith('.png') && !isIgnored(f)) names.add(f);
    }
    return [...names].sort();
}

function main(): void {
    const check = process.argv.includes('--check');
    const root = process.cwd();
    const family = pcbnewFamily(root);
    const prefixes = prefixEngineMap(root, family);

    let unmatched = 0;
    const screenshots = listBaselines(root).map((name) => {
        const stem = name.replace(/\.png$/i, '');
        const hit = prefixes.find((p) => stem === p.prefix || stem.startsWith(p.prefix));
        if (!hit) unmatched++;
        return { name, engine: hit?.engine ?? DEFAULT_ENGINE };
    });

    const manifest: Manifest & { _note: string } = {
        _note: 'engine tags are best-effort (gen-manifest.ts); the name list is authoritative. Refine engines after calibration.',
        screenshots,
    };
    const json = JSON.stringify(manifest, null, 2) + '\n';
    const outPath = path.join(root, MANIFEST_PATH);

    const dist = screenshots.reduce<Record<string, number>>((d, s) => ((d[s.engine] = (d[s.engine] ?? 0) + 1), d), {});
    console.log(`[manifest] ${screenshots.length} screenshots; engines=${JSON.stringify(dist)}; default-assigned=${unmatched}`);

    if (check) {
        const current = fs.existsSync(outPath) ? fs.readFileSync(outPath, 'utf8') : '';
        if (current !== json) {
            console.error('[manifest] STALE — run `npm run screenshots:manifest` and commit');
            process.exitCode = 1;
        } else {
            console.log('[manifest] up to date');
        }
        return;
    }
    fs.writeFileSync(outPath, json);
    console.log(`[manifest] wrote ${MANIFEST_PATH}`);
}

if (require.main === module) main();
