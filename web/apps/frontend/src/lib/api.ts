import {
  contract,
  type Project,
  type ProjectFile,
  type ProjectWithFiles,
  type UploadResponse,
} from "@kicad-web/contract";
import { initClient } from "@ts-rest/core";
import {
  useMutation,
  useQuery,
  useQueryClient,
} from "@tanstack/react-query";
import { API_BASE_URL } from "./config";

export const client = initClient(contract, {
  baseUrl: API_BASE_URL,
  baseHeaders: {},
});

// --- queries ---

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

// --- mutations ---

export function useCreateProject() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: async (input: {
      name: string;
      slug?: string;
    }): Promise<Project> => {
      const res = await client.createProject({ body: input });
      if (res.status === 409) throw new Error("a project with that slug exists");
      if (res.status !== 201) throw new Error("failed to create project");
      return res.body;
    },
    onSuccess: () => qc.invalidateQueries({ queryKey: ["projects"] }),
  });
}

export function useDeleteProject() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: async (slug: string): Promise<void> => {
      const res = await client.deleteProject({
        params: { project: slug },
        body: {},
      });
      if (res.status !== 200) throw new Error("failed to delete project");
    },
    onSuccess: () => qc.invalidateQueries({ queryKey: ["projects"] }),
  });
}

// --- raw binary endpoints (not in the ts-rest contract) ---

export interface UploadItem {
  /** project-relative path; folders use webkitRelativePath */
  path: string;
  file: File;
}

export async function uploadFiles(
  slug: string,
  items: UploadItem[],
): Promise<ProjectFile[]> {
  const form = new FormData();
  for (const item of items) {
    // Field name carries the relative path (server reads part.fieldname).
    form.append(item.path, item.file, item.file.name);
  }
  const res = await fetch(
    `${API_BASE_URL}/api/projects/${encodeURIComponent(slug)}/files`,
    { method: "POST", body: form },
  );
  if (!res.ok) throw new Error(`upload failed: ${res.status}`);
  return ((await res.json()) as UploadResponse).files;
}

export async function uploadZip(
  slug: string,
  zip: File,
): Promise<ProjectFile[]> {
  const form = new FormData();
  form.append("zip", zip, zip.name);
  const res = await fetch(
    `${API_BASE_URL}/api/projects/${encodeURIComponent(slug)}/files/zip`,
    { method: "POST", body: form },
  );
  if (!res.ok) throw new Error(`zip upload failed: ${res.status}`);
  return ((await res.json()) as UploadResponse).files;
}

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
