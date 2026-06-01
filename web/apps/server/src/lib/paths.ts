import * as path from "node:path";

/**
 * Normalize an arbitrary client-supplied relative path into a safe POSIX
 * project-relative path. Strips leading slashes and any `..` traversal.
 */
export function sanitizeRelPath(input: string): string {
  const posix = input.replace(/\\/g, "/");
  const normalized = path.posix
    .normalize(posix)
    .replace(/^(\.\.(\/|$))+/, "")
    .replace(/^\/+/, "");
  if (!normalized || normalized === "." || normalized.startsWith("..")) {
    throw new Error(`invalid file path: ${input}`);
  }
  return normalized;
}

export function slugify(name: string): string {
  const base = name
    .toLowerCase()
    .trim()
    .replace(/[^a-z0-9._-]+/g, "-")
    .replace(/^-+|-+$/g, "")
    .replace(/-{2,}/g, "-");
  return base || "project";
}

const TEXT_EXT = new Set([
  ".kicad_pcb",
  ".kicad_sch",
  ".kicad_pro",
  ".kicad_sym",
  ".kicad_mod",
  ".kicad_dru",
  ".kicad_wks",
  ".net",
  ".txt",
  ".csv",
  ".json",
]);

export function guessContentType(relPath: string): string {
  const ext = path.posix.extname(relPath).toLowerCase();
  if (TEXT_EXT.has(ext)) return "text/plain; charset=utf-8";
  return "application/octet-stream";
}
