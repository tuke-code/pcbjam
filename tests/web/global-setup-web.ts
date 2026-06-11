import type { FullConfig } from '@playwright/test';

/**
 * Global setup for the web-app e2e suite.
 *
 * The stack is the standalone editor (:3048) plus the reference backend
 * (:3060, @pcbjam/backend-example), which serves a single project off the
 * local filesystem — PROJECT_DIR=tests/fixtures/demo, so the slug is "demo"
 * (basename of PROJECT_DIR). Nothing to seed: just wait for the backend and
 * verify it serves the committed demo files the specs open.
 */

const API_BASE = process.env.BACKEND_URL ?? 'http://localhost:3060';
const DEMO_SLUG = 'demo';
const DEMO_FILES = ['demo.kicad_sch', 'demo.kicad_pcb', 'demo.kicad_wks'];

async function waitForApi(timeoutMs = 60000): Promise<void> {
  const deadline = Date.now() + timeoutMs;
  let lastErr = '';
  while (Date.now() < deadline) {
    try {
      const r = await fetch(`${API_BASE}/health`);
      if (r.ok) return;
      lastErr = `HTTP ${r.status}`;
    } catch (e) {
      lastErr = (e as Error).message;
    }
    await new Promise((res) => setTimeout(res, 1000));
  }
  throw new Error(
    `backend not reachable at ${API_BASE} (${lastErr}). Start the web stack first: ` +
      `from web/ run \`pnpm dev\` (PROJECT_DIR defaults to tests/fixtures/demo via ` +
      `backend/.env or the playwright webServer env).`
  );
}

async function verifyDemoProject(): Promise<void> {
  const r = await fetch(`${API_BASE}/api/projects/${DEMO_SLUG}`);
  if (!r.ok) {
    throw new Error(
      `GET /api/projects/${DEMO_SLUG} -> HTTP ${r.status}; ` +
        `is the backend's PROJECT_DIR pointing at tests/fixtures/demo?`
    );
  }
  const body = (await r.json()) as { files: { path: string }[] };
  const have = new Set(body.files.map((f) => f.path));
  const missing = DEMO_FILES.filter((f) => !have.has(f));
  if (missing.length) {
    throw new Error(`demo project missing files: ${missing.join(', ')}`);
  }
}

export default async function globalSetup(_config: FullConfig): Promise<void> {
  await waitForApi();
  await verifyDemoProject();
  console.log(`web e2e setup: backend at ${API_BASE} serving "${DEMO_SLUG}" — OK`);
}
