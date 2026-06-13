/**
 * pcbjam lib URIs are absolute POSIX paths under these mounts. Absolute so that
 * KiCad's lib-table URI expansion (ExpandURI -> wxFileName::MakeAbsolute) is a
 * no-op and the path reaches the plugin/provider unmangled — a "scheme://" URI
 * gets rewritten to "/scheme:/..." against the cwd, which differs per project.
 *
 * Two mount roots encode writability without an extra bridge round-trip (the
 * plugin's IsLibraryWritable is then a cheap prefix check):
 *   /mnt/pcbjam/<id>      read-only origins
 *   /mnt/pcbjam-rw/<id>   writable user libs
 */
export const PCBJAM_LIB_MOUNT = "/mnt/pcbjam";
export const PCBJAM_LIB_PREFIX = `${PCBJAM_LIB_MOUNT}/`;
export const PCBJAM_LIB_RW_MOUNT = "/mnt/pcbjam-rw";
export const PCBJAM_LIB_RW_PREFIX = `${PCBJAM_LIB_RW_MOUNT}/`;

/** The lib-table URI for a lib id (writable libs get the rw mount). */
export function libUri(id: string, writable = false): string {
  return `${writable ? PCBJAM_LIB_RW_PREFIX : PCBJAM_LIB_PREFIX}${id}`;
}

/** Recover the lib id from either mount's URI (arrives unmangled). */
export function libIdFromUri(uri: string): string | null {
  if (uri.startsWith(PCBJAM_LIB_RW_PREFIX))
    return uri.slice(PCBJAM_LIB_RW_PREFIX.length);
  if (uri.startsWith(PCBJAM_LIB_PREFIX))
    return uri.slice(PCBJAM_LIB_PREFIX.length);
  return null;
}
