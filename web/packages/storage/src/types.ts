import type { Readable } from "node:stream";

export interface StatResult {
  size: number;
  contentType?: string;
}

/**
 * Pluggable blob store for project file bytes.
 *
 * `key` is an opaque string the caller chose (see the server's storage-key
 * scheme); only the implementation interprets it. The current iteration is
 * read-heavy — the write half exists so save-back (a later iteration) needs no
 * redesign.
 */
export interface FileStorage {
  // --- read ---
  exists(key: string): Promise<boolean>;
  read(key: string): Promise<Uint8Array>;
  createReadStream(key: string): Readable;
  stat(key: string): Promise<StatResult>;
  list(prefix: string): Promise<string[]>;

  // --- write (used now by upload; save-back is a later iteration) ---
  write(
    key: string,
    data: Uint8Array | Readable,
    opts?: { contentType?: string },
  ): Promise<void>;
  delete(key: string): Promise<void>;
  /** Remove every key under a prefix (e.g. a whole project). */
  deletePrefix(prefix: string): Promise<void>;
}
