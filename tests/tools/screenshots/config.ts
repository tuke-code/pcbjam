/**
 * Central config for the screenshot regression + review tooling.
 *
 * The comparison engine (compare.ts), the churn-free updater (promote.ts) and
 * the Discord reporter (post-discord.ts) all read their knobs from here so
 * there is exactly one place to tune thresholds and paths.
 *
 * Paths are relative to the `tests/` directory (that is the working directory
 * the npm scripts and CI steps run from).
 */

/** Committed baseline directories, scanned in order. Filenames are the keys. */
export const BASELINE_DIRS = ['baseline-screenshots', 'e2e/baseline-screenshots'] as const;

/** Where Playwright writes the current run's screenshots (gitignored). */
export const RESULTS_DIR = 'test-results';

/** Where compare.ts writes diff/heatmap/triptych artifacts (gitignored). */
export const DIFF_OUT_DIR = 'test-results/screenshot-diff';

/** The manifest that records every expected screenshot + which engine renders it. */
export const MANIFEST_PATH = 'screenshot-manifest.json';

/**
 * pixelmatch per-pixel settings.
 *  - `threshold` is the YIQ perceptual distance (0..1) below which two pixels
 *    are considered equal. 0.1 tolerates gamma/AA jitter but catches real colour
 *    change.
 *  - `includeAA: false` (the pixelmatch default) means anti-aliased edge pixels
 *    are DETECTED AND IGNORED — exactly the sub-pixel/AA noise the old
 *    `maxDiff>16` counter was dominated by (see screenshot-compare.ts:36-43).
 */
export const PIXELMATCH = { threshold: 0.1, includeAA: false } as const;

/** Colour pixelmatch paints a real (non-AA) diff pixel with — the mask reads this back. */
export const DIFF_COLOR: [number, number, number] = [255, 0, 0];

/** Connected-component clustering ("where to look") parameters. */
export const CLUSTER = {
    dilate: 2, // grow the mask so fragmented glyph pixels merge into one box
    minBoxArea: 16, // drop specks smaller than this (px²)
    maxBoxes: 6, // draw at most this many (largest-first) red boxes
    boxColor: [255, 0, 0] as [number, number, number],
} as const;

/** Horizontal montage layout for the old | new+boxes | heatmap triptych. */
export const TRIPTYCH = {
    gap: 8,
    bg: [24, 24, 24, 255] as [number, number, number, number],
    padFill: [40, 0, 40, 255] as [number, number, number, number], // magenta pad on dim-mismatch
} as const;

/**
 * Per-engine verdict floors. A screenshot is CHANGED when its AA-excluded
 * changed-pixel ratio exceeds `changedRatio`. `meanChannelGuard` is recorded
 * for the drift-vs-regression heuristic (broad + low-intensity ⇒ environment
 * drift, not a localized regression), not for the primary verdict.
 *
 * NOTE: these are PLACEHOLDERS. `npm run screenshots:noise` renders the suite
 * twice on the CI host and prints the real intra-CI floor per engine; set
 * `changedRatio ≈ measured × 3` from that and commit the numbers here.
 */
export type EngineFloor = { changedRatio: number; meanChannelGuard: number };

export const FLOORS: Record<string, EngineFloor> = {
    'firefox-llvmpipe': { changedRatio: 0.002, meanChannelGuard: 2.0 },
    'chromium-swiftshader': { changedRatio: 0.002, meanChannelGuard: 2.0 },
    default: { changedRatio: 0.002, meanChannelGuard: 2.0 },
};

/**
 * Optional per-file rectangles to ignore before diffing (e.g. a live clock).
 * Keyed by screenshot filename. Empty for now.
 */
export const IGNORE_REGIONS: Record<string, Array<{ x: number; y: number; width: number; height: number }>> = {};

export type ManifestEntry = { name: string; engine: string };
export type Manifest = { screenshots: ManifestEntry[] };

/** Resolve the verdict floor for a screenshot via the manifest's engine tag. */
export function floorFor(name: string, manifest?: Manifest): EngineFloor {
    const engine = manifest?.screenshots.find((e) => e.name === name)?.engine;
    return (engine && FLOORS[engine]) || FLOORS.default;
}
