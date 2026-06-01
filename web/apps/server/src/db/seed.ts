import { eq } from "drizzle-orm";
import { env } from "../env.js";
import { db, pool } from "./index.js";
import { owners } from "./schema.js";

/** Ensure the default owner namespace exists (no-auth iteration). */
export async function seedDefaultOwner(): Promise<string> {
  const existing = await db
    .select()
    .from(owners)
    .where(eq(owners.slug, env.DEFAULT_OWNER_SLUG))
    .limit(1);
  if (existing[0]) return existing[0].id;

  const inserted = await db
    .insert(owners)
    .values({ slug: env.DEFAULT_OWNER_SLUG })
    .onConflictDoNothing()
    .returning();
  if (inserted[0]) return inserted[0].id;

  // Lost a race; re-read.
  const row = await db
    .select()
    .from(owners)
    .where(eq(owners.slug, env.DEFAULT_OWNER_SLUG))
    .limit(1);
  if (!row[0]) throw new Error("failed to seed default owner");
  return row[0].id;
}

// Allow running standalone: `pnpm db:seed`.
if (import.meta.url === `file://${process.argv[1]}`) {
  seedDefaultOwner()
    .then((id) => {
      console.log(`seeded default owner: ${id}`);
      return pool.end();
    })
    .catch((err) => {
      console.error(err);
      process.exit(1);
    });
}
