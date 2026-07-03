/**
 * Directory-pair screenshot comparison for the standalone regression suites
 * (first consumer: tests/3d-regression). Reuses the one comparison engine
 * (comparePair + image-ops) without touching the product gate in compare.ts —
 * that gate is welded to BASELINE_DIRS/RESULTS_DIR/manifest and must stay
 * byte-identical in behavior.
 *
 * Run modes (from the `tests/` directory):
 *   tsx tools/screenshots/compare-dirs.ts --old <dirA> --new <dirB> --out <diffDir> \
 *       [--floors <floors.json> --level <level>]   # per-suite floor config
 *       [--floor <changedRatio>]                   # ad-hoc floor (wins over --floors)
 *       [--fail-on-change]                         # exit 1 on changed/missing/extra
 *       [--label <text>]                           # caption suffix on triptychs
 *
 * The old dir is the reference side. A PNG present only in --old is MISSING,
 * only in --new is EXTRA; with a name-stable scenario registry both are
 * failures under --fail-on-change (an extra PNG means registry and baselines
 * diverged). Exit codes: 0 ok (or report-only), 1 fail-on-change tripped,
 * 2 usage/IO error.
 */
import * as fs from 'fs';
import * as path from 'path';
import { LABEL, labelText, type EngineFloor } from './config';
import { comparePair, type ChangedEntry } from './compare';
import { loadPng, savePng, withBottomLabel } from './image-ops';

const DEFAULT_FLOOR: EngineFloor = { changedRatio: 0.005, meanChannelGuard: 2.0 };

type LevelFloors = { default?: EngineFloor; overrides?: Record<string, EngineFloor> };
type FloorsFile = Record<string, LevelFloors>;

type DirReport = {
    old: string;
    new: string;
    level: string | null;
    changed: ChangedEntry[];
    /** present in --new only (kept as `added` to match compare.ts's Report shape) */
    added: Array<{ name: string; image: string }>;
    /** present in --old only */
    removed: Array<{ name: string; image: string }>;
    unchangedCount: number;
};

function listPngs(dir: string): string[] {
    return fs
        .readdirSync(dir)
        .filter((f) => f.toLowerCase().endsWith('.png'))
        .sort();
}

function parseArgs(argv: string[]): Record<string, string | boolean> {
    const out: Record<string, string | boolean> = {};
    for (let i = 0; i < argv.length; i++) {
        const a = argv[i];
        if (a === '--old') out.old = argv[++i];
        else if (a === '--new') out.new = argv[++i];
        else if (a === '--out') out.out = argv[++i];
        else if (a === '--floors') out.floors = argv[++i];
        else if (a === '--level') out.level = argv[++i];
        else if (a === '--floor') out.floor = argv[++i];
        else if (a === '--label') out.label = argv[++i];
        else if (a === '--fail-on-change') out.failOnChange = true;
        else {
            console.error(`[compare-dirs] unknown argument: ${a}`);
            process.exit(2);
        }
    }
    return out;
}

function usageError(msg: string): never {
    console.error(`[compare-dirs] ${msg}`);
    console.error(
        '[compare-dirs] usage: --old <dir> --new <dir> --out <dir> ' +
            '[--floors <json> --level <level>] [--floor <ratio>] [--fail-on-change] [--label <text>]'
    );
    process.exit(2);
}

/** Resolution: --floor (ad hoc) → floors.json per-name override → level default → built-in. */
function makeFloorResolver(args: Record<string, string | boolean>): (name: string) => EngineFloor {
    if (args.floor !== undefined) {
        const ratio = Number(args.floor);
        if (!Number.isFinite(ratio) || ratio < 0) usageError(`--floor must be a non-negative number, got "${args.floor}"`);
        const adHoc: EngineFloor = { changedRatio: ratio, meanChannelGuard: DEFAULT_FLOOR.meanChannelGuard };
        return () => adHoc;
    }
    if (args.floors === undefined) return () => DEFAULT_FLOOR;
    if (args.level === undefined) usageError('--floors requires --level');
    if (!fs.existsSync(args.floors as string)) usageError(`floors file not found: ${args.floors}`);
    let floorsFile: FloorsFile;
    try {
        floorsFile = JSON.parse(fs.readFileSync(args.floors as string, 'utf8'));
    } catch (e) {
        usageError(`could not parse ${args.floors}: ${(e as Error).message}`);
    }
    const level = floorsFile[args.level as string];
    if (!level) usageError(`level "${args.level}" not found in ${args.floors}`);
    return (name) => level.overrides?.[name] ?? level.default ?? DEFAULT_FLOOR;
}

function main(): void {
    const args = parseArgs(process.argv.slice(2));
    for (const req of ['old', 'new', 'out'] as const) {
        if (!args[req]) usageError(`--${req} is required`);
    }
    const oldDir = args.old as string;
    const newDir = args.new as string;
    const outDir = args.out as string;
    if (!fs.existsSync(oldDir)) usageError(`--old dir not found: ${oldDir}`);
    if (!fs.existsSync(newDir)) usageError(`--new dir not found: ${newDir}`);
    const label = (args.label as string) || null;
    const levelName = (args.level as string) || null;
    const floorFor = makeFloorResolver(args);

    fs.mkdirSync(outDir, { recursive: true });
    const oldNames = listPngs(oldDir);
    const newNames = new Set(listPngs(newDir));

    const report: DirReport = {
        old: oldDir,
        new: newDir,
        level: levelName,
        changed: [],
        added: [],
        removed: [],
        unchangedCount: 0,
    };

    const caption = (status: 'added' | 'removed' | 'changed', name: string) => labelText(status, name, label);

    for (const name of oldNames) {
        if (!newNames.has(name)) {
            const imagePath = path.join(outDir, `${name}.removed.png`);
            savePng(imagePath, withBottomLabel(loadPng(path.join(oldDir, name)), caption('removed', name), LABEL.colors.removed));
            report.removed.push({ name, image: imagePath });
            continue;
        }
        let pair: ReturnType<typeof comparePair>;
        try {
            pair = comparePair(
                loadPng(path.join(oldDir, name)),
                loadPng(path.join(newDir, name)),
                name,
                floorFor(name)
            );
        } catch (e) {
            console.error(`[compare-dirs] failed to compare ${name}: ${(e as Error).message}`);
            process.exit(2);
        }
        const { result, heatmap, triptych } = pair;
        if (result.verdict === 'unchanged') {
            report.unchangedCount++;
            continue;
        }
        const triptychPath = path.join(outDir, `${name}.triptych.png`);
        const heatmapPath = path.join(outDir, `${name}.heatmap.png`);
        savePng(triptychPath, withBottomLabel(triptych, caption('changed', name), LABEL.colors.changed));
        savePng(heatmapPath, heatmap);
        report.changed.push({ ...result, triptych: triptychPath, heatmap: heatmapPath });
    }

    for (const name of newNames) {
        if (oldNames.includes(name)) continue;
        const imagePath = path.join(outDir, `${name}.added.png`);
        savePng(imagePath, withBottomLabel(loadPng(path.join(newDir, name)), caption('added', name), LABEL.colors.added));
        report.added.push({ name, image: imagePath });
    }

    report.changed.sort((a, b) => b.changedRatio - a.changedRatio);
    fs.writeFileSync(path.join(outDir, 'report.json'), JSON.stringify(report, null, 2));

    console.log(
        `[compare-dirs]${levelName ? ` level=${levelName}` : ''} changed=${report.changed.length} ` +
            `missing=${report.removed.length} extra=${report.added.length} unchanged=${report.unchangedCount}`
    );
    for (const c of report.changed.slice(0, 10)) {
        console.log(`  CHANGED ${c.name} ratio=${(c.changedRatio * 100).toFixed(3)}% ${c.driftHint}`);
    }
    for (const r of report.removed) console.log(`  MISSING ${r.name} (in --old only)`);
    for (const a of report.added) console.log(`  EXTRA   ${a.name} (in --new only)`);

    if (args.failOnChange && (report.changed.length || report.added.length || report.removed.length)) {
        process.exitCode = 1;
    }
}

main();
