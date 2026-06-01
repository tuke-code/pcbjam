import { config } from "dotenv";
import * as path from "node:path";
import { fileURLToPath } from "node:url";
import { z } from "zod";

const here = path.dirname(fileURLToPath(import.meta.url));
// web/.env lives three levels up from apps/server/src. Standard precedence:
// real environment variables win over .env (don't override).
config({ path: path.resolve(here, "../../../.env") });

const envSchema = z.object({
  DATABASE_URL: z.string().min(1),
  PORT: z.coerce.number().int().positive().default(3050),
  STORAGE_DRIVER: z.string().default("local"),
  STORAGE_ROOT: z.string().default("./.data/storage"),
  CORS_ORIGIN: z.string().default("http://localhost:3048"),
  DEFAULT_OWNER_SLUG: z.string().default("default"),
});

export const env = envSchema.parse(process.env);
export type Env = typeof env;
