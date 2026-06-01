export type { FileStorage, StatResult } from "./types.js";
export { LocalDiskStorage } from "./local-disk.js";

import { LocalDiskStorage } from "./local-disk.js";
import type { FileStorage } from "./types.js";

/**
 * Build the storage backend from environment. Today only `local` is wired; an
 * `s3` driver slots in here later behind the same FileStorage interface.
 */
export function createFileStorage(env: {
  STORAGE_DRIVER?: string;
  STORAGE_ROOT?: string;
}): FileStorage {
  const driver = env.STORAGE_DRIVER ?? "local";
  switch (driver) {
    case "local":
      return new LocalDiskStorage(env.STORAGE_ROOT ?? "./.data/storage");
    default:
      throw new Error(`unknown STORAGE_DRIVER: ${driver}`);
  }
}
