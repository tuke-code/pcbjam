import { defineConfig } from "vitest/config";

// Node env by default (the waitlist route test). The boot.js DOM test opts into
// happy-dom per-file via a `// @vitest-environment happy-dom` pragma.
export default defineConfig({
  test: {
    include: ["test/**/*.test.ts"],
  },
});
