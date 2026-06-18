import type { Project, ProjectFile, ProjectWithFiles } from "@pcbjam/shared";
import {
  API_BASE_URL,
  PROJECT_MANIFEST_URL,
  PROJECT_SOURCE_KIND,
} from "./config";
import { client } from "./contract-client";

/**
 * Where the standalone gets its PROJECTS. The default `remote` source talks the
 * @pcbjam/shared REST contract; the `static` source serves a read-only example
 * gallery (manifest + file bytes) from a CDN with no backend — the
 * demo.pcbjam.com mode, where Save downloads to local (`uploadFileBytes` absent
 * ⇒ the caller downloads). One source is active per deployment, selected by
 * PROJECT_SOURCE_KIND. See docs/features/demo-deploy/.
 */
export interface ProjectSource {
  /** No write-back target — the editor should download saves to local. */
  readonly readOnly: boolean;
  listProjects(): Promise<Project[]>;
  getProject(slug: string): Promise<ProjectWithFiles>;
  fetchFileBytes(slug: string, relPath: string): Promise<Uint8Array>;
  /** Present only on writable sources; absent ⇒ read-only (download on save). */
  uploadFileBytes?(
    slug: string,
    relPath: string,
    bytes: Uint8Array,
  ): Promise<void>;
}

// --- remote (REST backend over the shared contract) ---------------------------

function encodePath(relPath: string): string {
  return relPath
    .split("/")
    .map((seg) => encodeURIComponent(seg))
    .join("/");
}

function remoteProjectSource(): ProjectSource {
  const fileUrl = (slug: string, relPath: string) =>
    `${API_BASE_URL}/api/projects/${encodeURIComponent(slug)}/files/${encodePath(relPath)}`;
  return {
    readOnly: false,
    async listProjects() {
      const res = await client.listProjects();
      if (res.status !== 200) throw new Error("failed to list projects");
      return res.body;
    },
    async getProject(slug) {
      const res = await client.getProject({ params: { project: slug } });
      if (res.status === 404) throw new Error("project not found");
      if (res.status !== 200) throw new Error("failed to load project");
      return res.body;
    },
    async fetchFileBytes(slug, relPath) {
      const res = await fetch(fileUrl(slug, relPath));
      if (!res.ok) throw new Error(`download failed (${res.status}): ${relPath}`);
      return new Uint8Array(await res.arrayBuffer());
    },
    async uploadFileBytes(slug, relPath, bytes) {
      const name = relPath.split("/").pop() ?? relPath;
      const form = new FormData();
      // The form FIELD NAME carries the project-relative path (upsert by
      // (project, path)) — same convention as the management app's folder upload.
      form.append(relPath, new File([bytes as BlobPart], name));
      const res = await fetch(
        `${API_BASE_URL}/api/projects/${encodeURIComponent(slug)}/files`,
        { method: "POST", body: form },
      );
      if (!res.ok) throw new Error(`upload failed (${res.status}): ${relPath}`);
    },
  };
}

// --- static (read-only gallery: manifest + file bytes on a CDN) ---------------

interface StaticManifestFile {
  path: string;
  size?: number;
}
interface StaticManifestProject {
  slug: string;
  name: string;
  description?: string;
  files: StaticManifestFile[];
}
interface StaticManifest {
  schema: number;
  tag: string;
  builtAt?: string;
  projects: StaticManifestProject[];
}

/** Stable v4-format UUID from a seed (cyrb128) — the contract ids are UUIDs and
 *  the editor uses project.id for the (broadcast-only here) collab room name, so
 *  a deterministic id keeps that stable across reloads/tabs. */
function deterministicUuid(seed: string): string {
  let h1 = 1779033703,
    h2 = 3144134277,
    h3 = 1013904242,
    h4 = 2773480762;
  for (let i = 0; i < seed.length; i++) {
    const k = seed.charCodeAt(i);
    h1 = h2 ^ Math.imul(h1 ^ k, 597399067);
    h2 = h3 ^ Math.imul(h2 ^ k, 2869860233);
    h3 = h4 ^ Math.imul(h3 ^ k, 951274213);
    h4 = h1 ^ Math.imul(h4 ^ k, 2716044179);
  }
  h1 = Math.imul(h3 ^ (h1 >>> 18), 597399067);
  h2 = Math.imul(h4 ^ (h2 >>> 22), 2869860233);
  h3 = Math.imul(h1 ^ (h3 >>> 17), 951274213);
  h4 = Math.imul(h2 ^ (h4 >>> 19), 2716044179);
  const hex = (n: number) => (n >>> 0).toString(16).padStart(8, "0");
  const h = hex(h1) + hex(h2) + hex(h3) + hex(h4);
  const variant = ((parseInt(h.charAt(16), 16) & 0x3) | 0x8).toString(16);
  return `${h.slice(0, 8)}-${h.slice(8, 12)}-4${h.slice(13, 16)}-${variant}${h.slice(17, 20)}-${h.slice(20, 32)}`;
}

function contentTypeFor(path: string): string {
  if (/\.(kicad_\w+|net|csv|pos|drl|gbr)$/i.test(path))
    return "text/plain; charset=utf-8";
  return "application/octet-stream";
}

function staticProjectSource(manifestUrl: string): ProjectSource {
  // Directory that holds the manifest — file bytes live at <dir>/<slug>/<path>.
  const baseDir = manifestUrl.replace(/\/[^/]*$/, "");
  let manifestP: Promise<StaticManifest> | null = null;
  const load = () =>
    (manifestP ??= (async () => {
      const res = await fetch(manifestUrl, { cache: "no-store" });
      if (!res.ok) throw new Error(`gallery manifest ${res.status}: ${manifestUrl}`);
      return (await res.json()) as StaticManifest;
    })());

  const toProject = (p: StaticManifestProject, ts: string): Project => ({
    id: deterministicUuid(`project:${p.slug}`),
    slug: p.slug,
    name: p.name,
    createdAt: ts,
    updatedAt: ts,
  });
  const toFile = (
    p: StaticManifestProject,
    f: StaticManifestFile,
    ts: string,
  ): ProjectFile => ({
    id: deterministicUuid(`file:${p.slug}/${f.path}`),
    projectId: deterministicUuid(`project:${p.slug}`),
    path: f.path,
    size: f.size ?? 0,
    contentType: contentTypeFor(f.path),
    createdAt: ts,
    updatedAt: ts,
  });

  const find = async (slug: string) => {
    const m = await load();
    const p = m.projects.find((x) => x.slug === slug);
    if (!p) throw new Error(`project not found: ${slug}`);
    return { m, p };
  };

  return {
    readOnly: true,
    async listProjects() {
      const m = await load();
      const ts = m.builtAt ?? new Date(0).toISOString();
      return m.projects.map((p) => toProject(p, ts));
    },
    async getProject(slug) {
      const { m, p } = await find(slug);
      const ts = m.builtAt ?? new Date(0).toISOString();
      return { project: toProject(p, ts), files: p.files.map((f) => toFile(p, f, ts)) };
    },
    async fetchFileBytes(slug, relPath) {
      const url = `${baseDir}/${encodeURIComponent(slug)}/${encodePath(relPath)}`;
      const res = await fetch(url);
      if (!res.ok) throw new Error(`download failed (${res.status}): ${relPath}`);
      return new Uint8Array(await res.arrayBuffer());
    },
    // No uploadFileBytes ⇒ read-only; the editor downloads saves to local.
  };
}

// --- selection ----------------------------------------------------------------

let cached: ProjectSource | null = null;

/** The active project source for this deployment (memoized). */
export function projectSource(): ProjectSource {
  if (cached) return cached;
  cached =
    PROJECT_SOURCE_KIND === "static" && PROJECT_MANIFEST_URL
      ? staticProjectSource(PROJECT_MANIFEST_URL)
      : remoteProjectSource();
  return cached;
}
