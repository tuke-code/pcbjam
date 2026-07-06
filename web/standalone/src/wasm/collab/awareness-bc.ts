import {
  applyAwarenessUpdate,
  encodeAwarenessUpdate,
  removeAwarenessStates,
  type Awareness,
} from "y-protocols/awareness";
import { clog } from "./debug";

/**
 * BroadcastChannel relay for a Yjs Awareness instance (collab-presence 0001):
 * the network providers (partykit/hocuspocus) multiplex awareness over their
 * own websocket, but the BroadcastChannel doc transport carries doc updates
 * only — this ferries the awareness sidecar between same-origin tabs so
 * presence works in the default zero-backend setup too.
 *
 * Same shape as `broadcast-transport.ts`: relay local update events out, apply
 * incoming ones with a marker origin so they don't echo back, and query peers
 * on join so a late tab sees existing states immediately. The Awareness
 * heartbeat (periodic local-clock bump) rides the same update event, so remote
 * expiry (~30s of silence) works unchanged.
 */

type Msg = { t: "aw"; u: Uint8Array } | { t: "aw-query" };

const BC_REMOTE_ORIGIN = "aw-bc-remote";

export function connectAwarenessBroadcast(
  awareness: Awareness,
  channelName: string,
): { destroy(): void } {
  const bc = new BroadcastChannel(channelName);
  clog("awareness BC open:", channelName);

  const onUpdate = (
    { added, updated, removed }: { added: number[]; updated: number[]; removed: number[] },
    origin: unknown,
  ) => {
    if (origin === BC_REMOTE_ORIGIN) return; // don't echo applied remote updates
    const changed = added.concat(updated, removed);
    if (!changed.length) return;
    bc.postMessage({ t: "aw", u: encodeAwarenessUpdate(awareness, changed) } satisfies Msg);
  };
  awareness.on("update", onUpdate);

  bc.onmessage = (e: MessageEvent<Msg>) => {
    const msg = e.data;
    if (!msg) return;
    switch (msg.t) {
      case "aw":
        applyAwarenessUpdate(awareness, new Uint8Array(msg.u), BC_REMOTE_ORIGIN);
        break;
      case "aw-query":
        if (awareness.getLocalState() !== null) {
          bc.postMessage({
            t: "aw",
            u: encodeAwarenessUpdate(awareness, [awareness.clientID]),
          } satisfies Msg);
        }
        break;
    }
  };

  // Ask existing tabs for their states (they answer with their own entry only;
  // every tab is on the channel, so the union covers the room).
  bc.postMessage({ t: "aw-query" } satisfies Msg);

  return {
    destroy: () => {
      // Broadcast our removal BEFORE detaching, so peers drop us immediately
      // instead of waiting out the awareness timeout.
      removeAwarenessStates(awareness, [awareness.clientID], "destroy");
      awareness.off("update", onUpdate);
      bc.close();
    },
  };
}
