import * as path from "node:path";
import { fileURLToPath } from "node:url";
import { migrate } from "drizzle-orm/node-postgres/migrator";
import { db, pool } from "./index.js";
import { seedDefaultOwner } from "./seed.js";

const here = path.dirname(fileURLToPath(import.meta.url));

async function main() {
  await migrate(db, {
    migrationsFolder: path.resolve(here, "../../drizzle"),
  });
  const ownerId = await seedDefaultOwner();
  console.log(`migrations applied; default owner: ${ownerId}`);
  await pool.end();
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
