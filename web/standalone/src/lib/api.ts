import type { DriftReportBody, Lib } from "@pcbjam/shared";
import { useQuery } from "@tanstack/react-query";
import { API_BASE_URL, PROJECT_SOURCE_KIND } from "./config";
import { client } from "./contract-client";
import { downloadBytes } from "./download";
import { projectSource } from "./project-source";

/**
 * Project/file reads go through the active PROJECT SOURCE (lib/project-source.ts):
 * the @pcbjam/shared REST backend, or the read-only static gallery (demo mode).
 * Libraries + collab drift reporting are backend-only and stay on the contract
 * client here.
 */

export function useProjects() {
  return useQuery({
    queryKey: ["projects"],
    queryFn: () => projectSource().listProjects(),
  });
}

/**
 * Libraries the backend serves, optionally filtered to a kind. In static (no
 * backend) mode there are none, so we short-circuit to an empty list rather than
 * fire a doomed request.
 */
export function useLibs(kind?: "symbol" | "footprint") {
  return useQuery({
    queryKey: ["libs", kind ?? "all"],
    queryFn: async (): Promise<Lib[]> => {
      if (PROJECT_SOURCE_KIND === "static") return [];
      const res = await client.listLibs({ query: { kind } });
      if (res.status !== 200) throw new Error("failed to list libraries");
      return res.body;
    },
  });
}

export function useProject(slug: string) {
  return useQuery({
    queryKey: ["project", slug],
    queryFn: () => projectSource().getProject(slug),
  });
}

/** File bytes from the active source (backend stream, or the static CDN gallery). */
export function fetchFileBytes(
  slug: string,
  relPath: string,
): Promise<Uint8Array> {
  return projectSource().fetchFileBytes(slug, relPath);
}

/**
 * Persist a saved file. A writable source (the backend) uploads it; a read-only
 * source (the static demo gallery) has no upload target, so the save downloads
 * to the user's machine instead. The remote-vs-static choice is config-driven
 * (the active project source), so callers just call this.
 */
export function uploadFileBytes(
  slug: string,
  relPath: string,
  bytes: Uint8Array,
): Promise<void> {
  const source = projectSource();
  if (source.uploadFileBytes) {
    return source.uploadFileBytes(slug, relPath, bytes);
  }
  downloadBytes(relPath, bytes);
  return Promise.resolve();
}

// --- collaboration drift reporting (ysync; backend-only) ---

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
