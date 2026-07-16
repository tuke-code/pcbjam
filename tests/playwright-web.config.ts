import { defineConfig, devices } from '@playwright/test';

/**
 * E2E config for the React WEB APP (web/standalone editor at :3048 + the
 * @pcbjam/backend-example reference backend at :3060), as opposed to
 * playwright-kicad.config.ts which drives the standalone tool harness HTMLs
 * under tests/apps/kicad.
 *
 * These tests exercise the real web open paths: navigate to /:scope/projects/:name/<file>,
 * let WasmTool boot the tool in-document (boot.ts), drive the project into MEMFS
 * and auto-open the file (open-flow.ts via Module.kicadOpenFile), and assert the
 * editor loaded wizard-free.
 *
 * The backend serves a single project off the local filesystem — no DB, no
 * seeding. The webServer block reuses an already-running stack (`pnpm dev` from
 * web/) or cold-starts it with the env below, pointing PROJECT_DIR at the
 * committed tests/fixtures/demo project (slug "demo").
 *
 * Firefox is the reliable headless target on ARM Mac (Chromium SwiftShader WebGL
 * bug); use --project=chromium (system Chrome) for headed debugging.
 */

const FRONTEND_URL = process.env.WEB_APP_URL ?? 'http://localhost:3048';
// Keep the cold-started stack consistent with WEB_APP_URL overrides (e.g. a
// sibling worktree squatting :3048): vite binds the URL's port and the backend
// allows that origin, so overriding one env var relocates the whole frontend.
const FRONTEND_PORT = new URL(FRONTEND_URL).port || '3048';
// Backend counterpart — same override story for :3060 squatters. BACKEND_URL
// is the var global-setup-web.ts already probes; the cold-started reference
// backend binds its port and the editor is pointed at it.
const BACKEND_URL = process.env.BACKEND_URL ?? 'http://localhost:3060';
const BACKEND_PORT = new URL(BACKEND_URL).port || '3060';

export default defineConfig({
  globalSetup: './web/global-setup-web.ts',
  testDir: './web',
  fullyParallel: false,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  // KiCad WASM is process-global (one runtime per page) and each tool wasm is
  // 40–200 MB; run serially to avoid loading several giant runtimes at once.
  workers: 1,
  reporter: 'html',
  timeout: 180000, // tool wasm download + boot + open can take minutes

  use: {
    baseURL: FRONTEND_URL,
    trace: 'retain-on-failure',
    screenshot: 'only-on-failure',
  },

  projects: [
    {
      name: 'firefox',
      testIgnore: /mobile-.*\.spec\.ts/,
      use: { ...devices['Desktop Firefox'], viewport: { width: 1280, height: 720 } },
    },
    {
      name: 'chromium',
      testIgnore: /mobile-.*\.spec\.ts/,
      use: { channel: 'chrome', viewport: { width: 1280, height: 720 } },
    },
    {
      // Canvas-only mobile mode (features/mobile). Mobile emulation is
      // Chromium-only, and headless SwiftShader WebGL is broken on ARM Mac (see
      // header) — run this project headed locally: --project=mobile-chromium --headed.
      name: 'mobile-chromium',
      testMatch: /mobile-.*\.spec\.ts/,
      use: { ...devices['Pixel 7'], channel: 'chrome' },
    },
  ],

  webServer: {
    // Best-effort: reuse the dev stack if it's already up (the common case);
    // otherwise cold-start turbo dev with the env a fresh checkout needs
    // (turbo passes these through, so no hand-copied .env files required).
    command: 'pnpm --dir ../web dev',
    url: FRONTEND_URL,
    reuseExistingServer: true,
    timeout: 120000,
    env: {
      ...process.env,
      // resolved against web/backend/ (the backend's cwd)
      PROJECT_DIR: '../../tests/fixtures/demo',
      PORT: BACKEND_PORT,
      VITE_API_BASE_URL: BACKEND_URL,
      CORS_ORIGIN: FRONTEND_URL,
      STANDALONE_PORT: FRONTEND_PORT,
      // The missing-file tool-switch spec builds a browser-local (IDB) project
      // through the home page — same flag the dev/demo stacks set
      // (scripts/dev-gpl.mjs). Only effective on cold starts: with
      // reuseExistingServer an already-running stack must have set it itself.
      VITE_LOCAL_PROJECTS: 'idb',
    },
  },
});
