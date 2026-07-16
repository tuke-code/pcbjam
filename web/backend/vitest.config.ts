import { defineConfig } from "vitest/config";

// The reference backend is a Fastify app; tests drive it via app.inject() in a
// plain node env (no network listen).
export default defineConfig({
  test: {
    environment: "node",
    include: ["test/**/*.test.ts"],
  },
});
