import cors from "@fastify/cors";
import multipart from "@fastify/multipart";
import Fastify, { type FastifyInstance } from "fastify";
import { env } from "./env.js";
import { apiPlugin } from "./routes/api.js";
import { fileRoutes } from "./routes/files.js";

export async function buildApp(): Promise<FastifyInstance> {
  const app = Fastify({
    logger: true,
    // Project files (whole KiCad trees) and zips can be large.
    bodyLimit: 1024 * 1024 * 1024,
  });

  await app.register(cors, {
    origin: env.CORS_ORIGIN === "*" ? true : env.CORS_ORIGIN.split(","),
  });

  await app.register(multipart, {
    limits: {
      fileSize: 1024 * 1024 * 1024, // 1 GiB per file
      files: 5000, // a KiCad project can have many lib files
    },
  });

  app.get("/health", async () => ({ ok: true }));

  await app.register(apiPlugin);
  await app.register(fileRoutes);

  return app;
}
