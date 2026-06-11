import { defineConfig, devices } from '@playwright/test';

/**
 * E2E config for the React WEB APP (web/standalone editor at :3048 + the
 * @pcbjam/backend-example reference backend at :3060), as opposed to
 * playwright-kicad.config.ts which drives the standalone tool harness HTMLs
 * under tests/apps/kicad.
 *
 * These tests exercise the real web open paths: navigate to /p/<project>/<tool>/<file>,
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
      use: { ...devices['Desktop Firefox'], viewport: { width: 1280, height: 720 } },
    },
    {
      name: 'chromium',
      use: { channel: 'chrome', viewport: { width: 1280, height: 720 } },
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
      VITE_API_BASE_URL: 'http://localhost:3060',
      CORS_ORIGIN: 'http://localhost:3048',
    },
  },
});
