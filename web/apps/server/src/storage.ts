import { createFileStorage } from "@kicad-web/storage";
import { env } from "./env.js";

export const storage = createFileStorage(env);

export function projectStoragePrefix(ownerId: string, projectId: string): string {
  return `owners/${ownerId}/projects/${projectId}`;
}

export function fileStorageKey(
  ownerId: string,
  projectId: string,
  relPath: string,
): string {
  return `${projectStoragePrefix(ownerId, projectId)}/${relPath}`;
}
