import type { PresencePeer } from "@/wasm/collab/presence";

/**
 * "Who else is in this file" (collab-presence 0001): a compact facepile of the
 * room's OTHER users, one colored initial-avatar per person, fed by the collab
 * session's awareness. Rendered in the editor's top-right overlay stack next to
 * SourceChip; the parent hides it when there are no peers. Chip styling mirrors
 * SourceChip (solid fill + inset ring) so it is legible on any backdrop.
 */
const MAX_AVATARS = 5;

export function PresenceRoster({ peers }: { peers: PresencePeer[] }) {
  if (!peers.length) return null;
  const names = peers.map((p) => p.user.name).join(", ");
  const shown = peers.slice(0, MAX_AVATARS);
  return (
    <span
      data-testid="presence-roster"
      title={`Also here: ${names}`}
      className="inline-flex items-center rounded-full bg-black/70 py-0.5 pl-1 pr-1.5 shadow-sm ring-1 ring-inset ring-white/20"
    >
      {shown.map((p) => (
        <span
          key={p.user.id}
          data-presence-user={p.user.id}
          title={p.user.name}
          style={{ backgroundColor: p.user.color }}
          className="-ml-1.5 flex h-5 w-5 items-center justify-center rounded-full text-[10px] font-semibold text-white ring-2 ring-black/50 first:ml-0"
        >
          {p.user.name.charAt(0).toUpperCase()}
        </span>
      ))}
      {peers.length > MAX_AVATARS && (
        <span className="ml-1 text-[10px] font-medium text-white/80">
          +{peers.length - MAX_AVATARS}
        </span>
      )}
    </span>
  );
}
