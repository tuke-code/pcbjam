import * as fs from "node:fs/promises";
import * as os from "node:os";
import * as path from "node:path";
import type { FastifyInstance } from "fastify";
import { afterAll, beforeAll, describe, expect, it, vi } from "vitest";

/**
 * Reference-backend hardening: write bounds (body size + per-owner/per-lib
 * quotas) and CORS. The app is exercised via Fastify's app.inject() — no network
 * listen. Env is set BEFORE the dynamic import so the module-level config
 * (bodyLimit, quotas, CORS) captures the test values.
 */
const USER = "x-pcbjam-user";

async function tmpdir(prefix: string): Promise<string> {
  return fs.mkdtemp(path.join(os.tmpdir(), prefix));
}

describe("reference backend write bounds + CORS", () => {
  let app: FastifyInstance;
  let projectDir: string;
  let userLibsDir: string;

  beforeAll(async () => {
    projectDir = await tmpdir("pcbjam-proj-");
    userLibsDir = await tmpdir("pcbjam-userlibs-");
    await fs.writeFile(path.join(projectDir, "board.kicad_pcb"), "(kicad_pcb)");
    process.env.PROJECT_DIR = projectDir;
    process.env.USER_LIBS_DIR = userLibsDir;
    process.env.MAX_ITEM_BYTES = "2048";
    process.env.MAX_LIBS_PER_OWNER = "2";
    process.env.CORS_ORIGIN = "*";
    process.env.NODE_ENV = "test";
    vi.resetModules();
    const { buildApp } = await import("../src/server.js");
    app = await buildApp();
    await app.ready();
  });

  afterAll(async () => {
    await app?.close();
    await fs.rm(projectDir, { recursive: true, force: true });
    await fs.rm(userLibsDir, { recursive: true, force: true });
  });

  it("rejects an item body over the size cap with 413", async () => {
    const oversized = "x".repeat(4096); // > MAX_ITEM_BYTES (2048)
    const res = await app.inject({
      method: "PUT",
      url: "/api/scopes/s/libs/anylib/items/symbol/Foo",
      headers: { "content-type": "text/plain", [USER]: "sizer" },
      payload: oversized,
    });
    expect(res.statusCode).toBe(413);
  });

  it("caps the number of libs an owner can create", async () => {
    const mk = (name: string) =>
      app.inject({
        method: "POST",
        url: "/api/scopes/s/libs",
        headers: { "content-type": "application/json", [USER]: "quota" },
        payload: { name },
      });
    expect((await mk("lib-a")).statusCode).toBe(201);
    expect((await mk("lib-b")).statusCode).toBe(201);
    const third = await mk("lib-c"); // exceeds MAX_LIBS_PER_OWNER (2)
    expect(third.statusCode).toBe(400);
    expect(third.json().message).toMatch(/limit/i);
  });

  it("a wildcard CORS origin does NOT also allow credentials", async () => {
    const res = await app.inject({
      method: "GET",
      url: "/health",
      headers: { origin: "https://other.example" },
    });
    // reflect-any-origin with allow-credentials is unsafe; the wildcard opt-in
    // must force credentials OFF.
    expect(res.headers["access-control-allow-credentials"]).not.toBe("true");
  });
});

describe("reference backend CORS — explicit origin still allows credentials", () => {
  it("a configured origin keeps credentials on", async () => {
    const projectDir = await tmpdir("pcbjam-proj2-");
    await fs.writeFile(path.join(projectDir, "board.kicad_pcb"), "(kicad_pcb)");
    process.env.PROJECT_DIR = projectDir;
    process.env.USER_LIBS_DIR = await tmpdir("pcbjam-userlibs2-");
    process.env.CORS_ORIGIN = "http://localhost:3048";
    process.env.NODE_ENV = "test";
    vi.resetModules();
    const { buildApp } = await import("../src/server.js");
    const app = await buildApp();
    await app.ready();
    try {
      const res = await app.inject({
        method: "GET",
        url: "/health",
        headers: { origin: "http://localhost:3048" },
      });
      expect(res.headers["access-control-allow-credentials"]).toBe("true");
      expect(res.headers["access-control-allow-origin"]).toBe("http://localhost:3048");
    } finally {
      await app.close();
      await fs.rm(projectDir, { recursive: true, force: true });
    }
  });
});
