import {
  contract,
  type DriftReportBody,
  type Lib,
  type Project,
  type ProjectWithFiles,
} from "@pcbjam/shared";
import { initClient } from "@ts-rest/core";
import { useQuery } from "@tanstack/react-query";
import { API_BASE_URL } from "./config";

/**
 * Client over the shared contract. The standalone editor READS projects from a
 * backend (enumerate, get file tree, stream bytes) and writes back exactly one
 * thing: the bytes of a file the user explicitly saved in the editor (see
 * uploadFileBytes). Project management (create/delete/bulk upload) stays in the
 * closed application that hosts this editor.
 */
export const client = initClient(contract, {
  baseUrl: API_BASE_URL,
  baseHeaders: {},
});

export function useProjects() {
  return useQuery({
    queryKey: ["projects"],
    queryFn: async (): Promise<Project[]> => {
      const res = await client.listProjects();
      if (res.status !== 200) throw new Error("failed to list projects");
      return res.body;
    },
  });
}

/**
 * Libraries the backend serves, optionally filtered to a kind ("symbol" |
 * "footprint"). Origins are kind-filtered server-side; user libs are
 * kind-agnostic and always returned. Mirrors `useProjects` — read-only listing
 * for the home page; the editor consumes libs over its own WASM bridge.
 */
export function useLibs(kind?: "symbol" | "footprint") {
  return useQuery({
    queryKey: ["libs", kind ?? "all"],
    queryFn: async (): Promise<Lib[]> => {
      const res = await client.listLibs({ query: { kind } });
      if (res.status !== 200) throw new Error("failed to list libraries");
      return res.body;
    },
  });
}

export function useProject(slug: string) {
  return useQuery({
    queryKey: ["project", slug],
    queryFn: async (): Promise<ProjectWithFiles> => {
      const res = await client.getProject({ params: { project: slug } });
      if (res.status === 404) throw new Error("project not found");
      if (res.status !== 200) throw new Error("failed to load project");
      return res.body;
    },
  });
}

// --- raw file-byte download (streamed binary, not a ts-rest endpoint) ---

export function fileBytesUrl(slug: string, relPath: string): string {
  const encoded = relPath
    .split("/")
    .map((seg) => encodeURIComponent(seg))
    .join("/");
  return `${API_BASE_URL}/api/projects/${encodeURIComponent(slug)}/files/${encoded}`;
}

export async function fetchFileBytes(
  slug: string,
  relPath: string,
): Promise<Uint8Array> {
  const res = await fetch(fileBytesUrl(slug, relPath));
  if (!res.ok) throw new Error(`download failed (${res.status}): ${relPath}`);
  return new Uint8Array(await res.arrayBuffer());
}

// --- collaboration drift reporting (ysync) ---

/**
 * Report a detected ydoc/wasm drift (the editor's periodic, every-N-edits check).
 * Best-effort: a failed report must never disrupt editing, so callers ignore
 * rejections.
 */
export async function reportDrift(
  slug: string,
  body: DriftReportBody,
): Promise<void> {
  await client.reportDrift({ params: { project: slug }, body });
}

/**
 * Fire-and-forget drift report that survives the page closing — used by the
 * session-end (`beforeunload`) check. `sendBeacon` queues the POST past unload;
 * a keepalive `fetch` is the fallback when the beacon is rejected (too large).
 */
export function reportDriftBeacon(slug: string, body: DriftReportBody): void {
  const url = `${API_BASE_URL}/api/projects/${encodeURIComponent(slug)}/drift`;
  const blob = new Blob([JSON.stringify(body)], { type: "application/json" });
  try {
    if (navigator.sendBeacon(url, blob)) return;
  } catch {
    /* fall through to keepalive fetch */
  }
  void fetch(url, { method: "POST", body: blob, keepalive: true }).catch(() => {});
}

/**
 * Persist one saved file back to the backend via the multipart upload route
 * (POST /api/projects/:project/files — upserts by (project, path); the form
 * FIELD NAME carries the project-relative path, same convention as the
 * management app's folder upload).
 */
export async function uploadFileBytes(
  slug: string,
  relPath: string,
  bytes: Uint8Array,
): Promise<void> {
  const name = relPath.split("/").pop() ?? relPath;
  const form = new FormData();
  form.append(relPath, new File([bytes as BlobPart], name));
  const res = await fetch(
    `${API_BASE_URL}/api/projects/${encodeURIComponent(slug)}/files`,
    { method: "POST", body: form },
  );
  if (!res.ok) throw new Error(`upload failed (${res.status}): ${relPath}`);
}
