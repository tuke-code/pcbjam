import type { Project, ProjectFile } from "@kicad-web/contract";
import type { Readable } from "node:stream";
import { and, asc, eq } from "drizzle-orm";
import { db } from "../db/index.js";
import {
  projectFiles,
  projects,
  type ProjectFileRow,
  type ProjectRow,
} from "../db/schema.js";
import { env } from "../env.js";
import { owners } from "../db/schema.js";
import { guessContentType, sanitizeRelPath, slugify } from "../lib/paths.js";
import { fileStorageKey, projectStoragePrefix, storage } from "../storage.js";

export class SlugConflictError extends Error {
  constructor(slug: string) {
    super(`project slug already exists: ${slug}`);
    this.name = "SlugConflictError";
  }
}

let cachedOwnerId: string | null = null;

export async function getDefaultOwnerId(): Promise<string> {
  if (cachedOwnerId) return cachedOwnerId;
  const row = await db
    .select()
    .from(owners)
    .where(eq(owners.slug, env.DEFAULT_OWNER_SLUG))
    .limit(1);
  if (!row[0]) {
    throw new Error(
      `default owner "${env.DEFAULT_OWNER_SLUG}" not found — run db:migrate`,
    );
  }
  cachedOwnerId = row[0].id;
  return cachedOwnerId;
}

function toApiProject(row: ProjectRow): Project {
  return {
    id: row.id,
    ownerId: row.ownerId,
    slug: row.slug,
    name: row.name,
    createdAt: row.createdAt.toISOString(),
    updatedAt: row.updatedAt.toISOString(),
  };
}

function toApiFile(row: ProjectFileRow): ProjectFile {
  return {
    id: row.id,
    projectId: row.projectId,
    path: row.path,
    size: row.size,
    contentType: row.contentType,
    createdAt: row.createdAt.toISOString(),
    updatedAt: row.updatedAt.toISOString(),
  };
}

export async function listProjects(): Promise<Project[]> {
  const ownerId = await getDefaultOwnerId();
  const rows = await db
    .select()
    .from(projects)
    .where(eq(projects.ownerId, ownerId))
    .orderBy(asc(projects.createdAt));
  return rows.map(toApiProject);
}

async function slugExists(ownerId: string, slug: string): Promise<boolean> {
  const row = await db
    .select({ id: projects.id })
    .from(projects)
    .where(and(eq(projects.ownerId, ownerId), eq(projects.slug, slug)))
    .limit(1);
  return !!row[0];
}

export async function createProject(
  name: string,
  slug?: string,
): Promise<Project> {
  const ownerId = await getDefaultOwnerId();

  if (slug) {
    if (await slugExists(ownerId, slug)) throw new SlugConflictError(slug);
  } else {
    const base = slugify(name);
    slug = base;
    for (let i = 2; await slugExists(ownerId, slug); i++) {
      slug = `${base}-${i}`;
    }
  }

  const inserted = await db
    .insert(projects)
    .values({ ownerId, slug, name })
    .returning();
  return toApiProject(inserted[0]!);
}

export async function getProjectRowBySlug(
  slug: string,
): Promise<ProjectRow | null> {
  const ownerId = await getDefaultOwnerId();
  const row = await db
    .select()
    .from(projects)
    .where(and(eq(projects.ownerId, ownerId), eq(projects.slug, slug)))
    .limit(1);
  return row[0] ?? null;
}

export async function listFiles(projectId: string): Promise<ProjectFileRow[]> {
  return db
    .select()
    .from(projectFiles)
    .where(eq(projectFiles.projectId, projectId))
    .orderBy(asc(projectFiles.path));
}

export async function listFilesApi(projectId: string): Promise<ProjectFile[]> {
  return (await listFiles(projectId)).map(toApiFile);
}

export async function getProjectWithFiles(slug: string): Promise<{
  project: Project;
  files: ProjectFile[];
} | null> {
  const row = await getProjectRowBySlug(slug);
  if (!row) return null;
  const files = await listFiles(row.id);
  return { project: toApiProject(row), files: files.map(toApiFile) };
}

export async function getFileRow(
  projectId: string,
  relPath: string,
): Promise<ProjectFileRow | null> {
  const row = await db
    .select()
    .from(projectFiles)
    .where(
      and(eq(projectFiles.projectId, projectId), eq(projectFiles.path, relPath)),
    )
    .limit(1);
  return row[0] ?? null;
}

export async function deleteProject(slug: string): Promise<string | null> {
  const row = await getProjectRowBySlug(slug);
  if (!row) return null;
  // Cascade removes project_file rows; storage prefix removed explicitly.
  await db.delete(projects).where(eq(projects.id, row.id));
  await storage.deletePrefix(projectStoragePrefix(row.ownerId, row.id));
  return row.id;
}

/**
 * Stream/buffer a single file into storage and upsert its index row. Used by
 * the upload routes (multi-file, folder, and zip entries).
 */
export async function writeProjectFile(opts: {
  project: ProjectRow;
  rawPath: string;
  data: Uint8Array | Readable;
  size?: number;
  contentType?: string;
}): Promise<ProjectFile> {
  const relPath = sanitizeRelPath(opts.rawPath);
  const key = fileStorageKey(opts.project.ownerId, opts.project.id, relPath);
  await storage.write(key, opts.data, { contentType: opts.contentType });

  const size = opts.size ?? (await storage.stat(key)).size;
  const contentType = opts.contentType ?? guessContentType(relPath);

  const inserted = await db
    .insert(projectFiles)
    .values({
      projectId: opts.project.id,
      path: relPath,
      size,
      contentType,
      storageKey: key,
    })
    .onConflictDoUpdate({
      target: [projectFiles.projectId, projectFiles.path],
      set: { size, contentType, storageKey: key, updatedAt: new Date() },
    })
    .returning();
  return toApiFile(inserted[0]!);
}
