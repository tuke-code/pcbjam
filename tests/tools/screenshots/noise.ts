/**
 * Calibration: measure the intra-CI noise floor.
 *
 * Render the suite twice on the same host (into two dirs), then:
 *   tsx tools/screenshots/noise.ts <run1-dir> <run2-dir>
 * prints, per engine (via the manifest) and globally, the max / p95 / mean
 * changed-pixel ratio between the two identical-input renders. Set
 * config.ts FLOORS.<engine>.changedRatio ≈ (max × 3) from this.
 *
 * A near-zero floor means the render is deterministic enough to catch real
 * changes tightly; a large floor exposes nondeterminism to chase before the
 * gate can be trusted.
 */
import * as fs from 'fs';
import * as path from 'path';
import { MANIFEST_PATH, type Manifest } from './config';
import { diffImages, loadPng } from './image-ops';

function listPngs(dir: string): string[] {
    return fs.existsSync(dir) ? fs.readdirSync(dir).filter((f) => f.toLowerCase().endsWith('.png')) : [];
}

function engineOf(name: string, manifest?: Manifest): string {
    return manifest?.screenshots.find((e) => e.name === name)?.engine ?? 'default';
}

function stats(values: number[]): { n: number; max: number; p95: number; mean: number } {
    if (!values.length) return { n: 0, max: 0, p95: 0, mean: 0 };
    const sorted = [...values].sort((a, b) => a - b);
    const p95 = sorted[Math.min(sorted.length - 1, Math.floor(sorted.length * 0.95))];
    return {
        n: values.length,
        max: sorted[sorted.length - 1],
        p95,
        mean: values.reduce((s, v) => s + v, 0) / values.length,
    };
}

function main(): void {
    const [dir1, dir2] = process.argv.slice(2);
    if (!dir1 || !dir2) {
        console.error('usage: noise.ts <run1-dir> <run2-dir>');
        process.exitCode = 2;
        return;
    }
    const root = process.cwd();
    const manifestPath = path.join(root, MANIFEST_PATH);
    const manifest = fs.existsSync(manifestPath)
        ? (JSON.parse(fs.readFileSync(manifestPath, 'utf8')) as Manifest)
        : undefined;

    const common = listPngs(dir1).filter((n) => fs.existsSync(path.join(dir2, n)));
    const byEngine = new Map<string, number[]>();
    const all: number[] = [];
    for (const name of common) {
        const d = diffImages(loadPng(path.join(dir1, name)), loadPng(path.join(dir2, name)));
        const ratio = d.dimsMatch ? d.changedRatio : 1;
        all.push(ratio);
        const eng = engineOf(name, manifest);
        (byEngine.get(eng) ?? byEngine.set(eng, []).get(eng)!).push(ratio);
    }

    const report = (label: string, values: number[]) => {
        const s = stats(values);
        console.log(
            `${label.padEnd(24)} n=${String(s.n).padStart(4)}  max=${(s.max * 100).toFixed(4)}%  ` +
                `p95=${(s.p95 * 100).toFixed(4)}%  mean=${(s.mean * 100).toFixed(4)}%  ` +
                `→ suggest changedRatio=${(s.max * 3).toExponential(2)}`
        );
    };
    console.log(`[noise] compared ${common.length} screenshots rendered twice`);
    for (const [eng, values] of byEngine) report(eng, values);
    report('ALL', all);
}

if (require.main === module) main();
