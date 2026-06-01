import {
  bigint,
  pgTable,
  text,
  timestamp,
  unique,
  uuid,
} from "drizzle-orm/pg-core";

export const owners = pgTable("owner", {
  id: uuid("id").primaryKey().defaultRandom(),
  slug: text("slug").notNull().unique(),
  createdAt: timestamp("created_at", { withTimezone: true })
    .notNull()
    .defaultNow(),
});

export const projects = pgTable(
  "project",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    ownerId: uuid("owner_id")
      .notNull()
      .references(() => owners.id, { onDelete: "cascade" }),
    slug: text("slug").notNull(),
    name: text("name").notNull(),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    updatedAt: timestamp("updated_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [unique("project_owner_slug_uq").on(t.ownerId, t.slug)],
);

export const projectFiles = pgTable(
  "project_file",
  {
    id: uuid("id").primaryKey().defaultRandom(),
    projectId: uuid("project_id")
      .notNull()
      .references(() => projects.id, { onDelete: "cascade" }),
    // POSIX project-relative path, e.g. "pcbnew/nyak.kicad_pcb".
    path: text("path").notNull(),
    size: bigint("size", { mode: "number" }).notNull(),
    contentType: text("content_type").notNull(),
    // Opaque key handed to FileStorage; decouples logical path from blob layout.
    storageKey: text("storage_key").notNull(),
    createdAt: timestamp("created_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
    updatedAt: timestamp("updated_at", { withTimezone: true })
      .notNull()
      .defaultNow(),
  },
  (t) => [unique("project_file_path_uq").on(t.projectId, t.path)],
);

export type OwnerRow = typeof owners.$inferSelect;
export type ProjectRow = typeof projects.$inferSelect;
export type ProjectFileRow = typeof projectFiles.$inferSelect;
