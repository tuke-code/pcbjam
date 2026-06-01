import { createReadStream as fsCreateReadStream } from "node:fs";
import * as fs from "node:fs/promises";
import * as path from "node:path";
import type { Readable } from "node:stream";
import { pipeline } from "node:stream/promises";
import { createWriteStream } from "node:fs";
import type { FileStorage, StatResult } from "./types.js";

/**
 * Stores blobs as files under a single root directory. `key` maps directly to a
 * relative path inside the root; traversal outside the root is rejected.
 */
export class LocalDiskStorage implements FileStorage {
  private readonly root: string;

  constructor(root: string) {
    this.root = path.resolve(root);
  }

  private resolve(key: string): string {
    const normalized = path
      .normalize(key)
      .replace(/^(\.\.(\/|\\|$))+/, "")
      .replace(/^[/\\]+/, "");
    const full = path.resolve(this.root, normalized);
    if (full !== this.root && !full.startsWith(this.root + path.sep)) {
      throw new Error(`storage key escapes root: ${key}`);
    }
    return full;
  }

  async exists(key: string): Promise<boolean> {
    try {
      await fs.access(this.resolve(key));
      return true;
    } catch {
      return false;
    }
  }

  async read(key: string): Promise<Uint8Array> {
    return new Uint8Array(await fs.readFile(this.resolve(key)));
  }

  createReadStream(key: string): Readable {
    return fsCreateReadStream(this.resolve(key));
  }

  async stat(key: string): Promise<StatResult> {
    const s = await fs.stat(this.resolve(key));
    return { size: s.size };
  }

  async list(prefix: string): Promise<string[]> {
    const base = this.resolve(prefix);
    const out: string[] = [];
    const walk = async (dir: string): Promise<void> => {
      let entries: import("node:fs").Dirent[];
      try {
        entries = await fs.readdir(dir, { withFileTypes: true });
      } catch {
        return;
      }
      for (const entry of entries) {
        const abs = path.join(dir, entry.name);
        if (entry.isDirectory()) {
          await walk(abs);
        } else if (entry.isFile()) {
          out.push(path.relative(this.root, abs).split(path.sep).join("/"));
        }
      }
    };
    await walk(base);
    return out;
  }

  async write(
    key: string,
    data: Uint8Array | Readable,
    _opts?: { contentType?: string },
  ): Promise<void> {
    const full = this.resolve(key);
    await fs.mkdir(path.dirname(full), { recursive: true });
    if (data instanceof Uint8Array) {
      await fs.writeFile(full, data);
    } else {
      await pipeline(data, createWriteStream(full));
    }
  }

  async delete(key: string): Promise<void> {
    await fs.rm(this.resolve(key), { force: true });
  }

  async deletePrefix(prefix: string): Promise<void> {
    await fs.rm(this.resolve(prefix), { recursive: true, force: true });
  }
}
