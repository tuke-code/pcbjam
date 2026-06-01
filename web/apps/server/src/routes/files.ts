import { randomUUID } from "node:crypto";
import { createWriteStream } from "node:fs";
import * as fs from "node:fs/promises";
import * as os from "node:os";
import * as path from "node:path";
import { pipeline } from "node:stream/promises";
import type { ProjectFile } from "@kicad-web/contract";
import type { FastifyInstance } from "fastify";
import unzipper from "unzipper";
import { sanitizeRelPath } from "../lib/paths.js";
import * as svc from "../services/projects.js";
import { storage } from "../storage.js";

/**
 * Binary file routes that don't round-trip cleanly through ts-rest:
 *   POST   /api/projects/:project/files       (multipart; multi-file + folder)
 *   POST   /api/projects/:project/files/zip   (multipart; one zip)
 *   GET    /api/projects/:project/files/*      (stream raw bytes)
 *
 * For multi-file / folder uploads the client sends each file as a part whose
 * FIELD NAME is the project-relative path (folder uploads pass
 * webkitRelativePath). The zip route unpacks entries preserving their paths.
 */
export async function fileRoutes(app: FastifyInstance): Promise<void> {
  // --- multi-file / folder upload ---
  app.post("/api/projects/:project/files", async (req, reply) => {
    const slug = (req.params as { project: string }).project;
    const project = await svc.getProjectRowBySlug(slug);
    if (!project) {
      return reply.code(404).send({ message: "project not found" });
    }

    const written: ProjectFile[] = [];
    for await (const part of req.parts()) {
      if (part.type !== "file") continue;
      // Field name carries the relative path; fall back to the filename.
      const rawPath = part.fieldname || part.filename || "";
      if (!rawPath) {
        part.file.resume();
        continue;
      }
      written.push(
        await svc.writeProjectFile({
          project,
          rawPath,
          data: part.file,
          contentType: part.mimetype,
        }),
      );
    }
    return reply.code(201).send({ files: written });
  });

  // --- zip upload ---
  app.post("/api/projects/:project/files/zip", async (req, reply) => {
    const slug = (req.params as { project: string }).project;
    const project = await svc.getProjectRowBySlug(slug);
    if (!project) {
      return reply.code(404).send({ message: "project not found" });
    }

    const zipPart = await req.file();
    if (!zipPart) {
      return reply.code(400).send({ message: "no zip file in request" });
    }

    const tmp = path.join(os.tmpdir(), `kicad-upload-${randomUUID()}.zip`);
    const written: ProjectFile[] = [];
    try {
      await pipeline(zipPart.file, createWriteStream(tmp));
      const directory = await unzipper.Open.file(tmp);
      for (const entry of directory.files) {
        if (entry.type !== "File") continue;
        let relPath: string;
        try {
          relPath = sanitizeRelPath(entry.path);
        } catch {
          continue; // skip traversal / invalid entries
        }
        written.push(
          await svc.writeProjectFile({
            project,
            rawPath: relPath,
            data: entry.stream(),
            size: entry.uncompressedSize,
          }),
        );
      }
    } finally {
      await fs.rm(tmp, { force: true });
    }
    return reply.code(201).send({ files: written });
  });

  // --- download raw bytes ---
  app.get("/api/projects/:project/files/*", async (req, reply) => {
    const params = req.params as { project: string; "*": string };
    const project = await svc.getProjectRowBySlug(params.project);
    if (!project) {
      return reply.code(404).send({ message: "project not found" });
    }
    let relPath: string;
    try {
      relPath = sanitizeRelPath(params["*"]);
    } catch {
      return reply.code(400).send({ message: "invalid path" });
    }
    const file = await svc.getFileRow(project.id, relPath);
    if (!file) {
      return reply.code(404).send({ message: "file not found" });
    }
    reply.header("Content-Type", file.contentType);
    reply.header("Content-Length", file.size);
    reply.header("Cache-Control", "no-cache");
    return reply.send(storage.createReadStream(file.storageKey));
  });
}
