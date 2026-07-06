import { afterEach, describe, expect, it } from "vitest";
import * as Y from "yjs";
import { Awareness } from "y-protocols/awareness";
import { colorForUser, type PresenceUser } from "@pcbjam/shared";
import { connectAwarenessBroadcast } from "./awareness-bc";
import { createPresence, type PresenceHandle } from "./presence";

/**
 * Presence unit tests (collab-presence 0001): two Awareness instances relayed
 * over a real BroadcastChannel (node ≥18 has it globally) stand in for two
 * tabs. BC delivery is async — `settle()` lets a posted message round-trip.
 */

const settle = () => new Promise((r) => setTimeout(r, 50));

function user(id: string): PresenceUser {
  return { id, name: id, color: colorForUser(id) };
}

interface Client {
  awareness: Awareness;
  presence?: PresenceHandle;
  destroy(): void;
}

let clients: Client[] = [];
let channelSeq = 0;

function client(channel: string): Client {
  const doc = new Y.Doc();
  const awareness = new Awareness(doc);
  const relay = connectAwarenessBroadcast(awareness, channel);
  const c: Client = {
    awareness,
    destroy() {
      this.presence?.destroy();
      relay.destroy();
      awareness.destroy();
      doc.destroy();
    },
  };
  clients.push(c);
  return c;
}

afterEach(() => {
  for (const c of clients) c.destroy();
  clients = [];
});

describe("presence over the BroadcastChannel awareness relay", () => {
  it("peers see each other's state; destroy removes it immediately", async () => {
    const channel = `presence-test-${channelSeq++}`;
    const a = client(channel);
    const b = client(channel);

    a.presence = createPresence({ awareness: a.awareness, user: user("alice"), tool: "pcbnew" });
    b.presence = createPresence({ awareness: b.awareness, user: user("bob"), tool: "pcbnew" });
    await settle();

    expect(a.presence.peers().map((p) => p.user.id)).toEqual(["bob"]);
    expect(b.presence.peers().map((p) => p.user.id)).toEqual(["alice"]);
    expect(a.presence.peers()[0]!.cursor).toBeNull();

    // destroy() nulls the local state → relayed → bob leaves alice's roster
    // without waiting for the awareness timeout.
    b.presence.destroy();
    b.presence = undefined;
    await settle();
    expect(a.presence.peers()).toEqual([]);
  });

  it("a late-joining tab sees existing peers (join query)", async () => {
    const channel = `presence-test-${channelSeq++}`;
    const a = client(channel);
    a.presence = createPresence({ awareness: a.awareness, user: user("alice"), tool: "pcbnew" });
    await settle();

    const b = client(channel);
    b.presence = createPresence({ awareness: b.awareness, user: user("bob"), tool: "pcbnew" });
    await settle();
    expect(b.presence.peers().map((p) => p.user.id)).toEqual(["alice"]);
  });

  it("dedupes the same user across tabs and hides own slug", async () => {
    const channel = `presence-test-${channelSeq++}`;
    const a1 = client(channel);
    const a2 = client(channel);
    const b = client(channel);
    a1.presence = createPresence({ awareness: a1.awareness, user: user("alice"), tool: "pcbnew" });
    a2.presence = createPresence({ awareness: a2.awareness, user: user("alice"), tool: "pcbnew" });
    b.presence = createPresence({ awareness: b.awareness, user: user("bob"), tool: "pcbnew" });
    await settle();

    // bob sees ONE alice (two tabs, one person); alice's tabs don't see each other.
    expect(b.presence.peers().map((p) => p.user.id)).toEqual(["alice"]);
    expect(a1.presence.peers().map((p) => p.user.id)).toEqual(["bob"]);
  });

  it("drops malformed peer states instead of crashing the roster", async () => {
    const channel = `presence-test-${channelSeq++}`;
    const a = client(channel);
    const b = client(channel);
    a.presence = createPresence({ awareness: a.awareness, user: user("alice"), tool: "pcbnew" });
    b.awareness.setLocalState({ some: "legacy-shape" });
    await settle();
    expect(a.presence.peers()).toEqual([]);
  });

  it("subscribe fires on change and setters keep sibling fields intact", async () => {
    const channel = `presence-test-${channelSeq++}`;
    const a = client(channel);
    const b = client(channel);
    a.presence = createPresence({ awareness: a.awareness, user: user("alice"), tool: "pcbnew" });
    b.presence = createPresence({ awareness: b.awareness, user: user("bob"), tool: "pcbnew" });
    await settle();

    let latest: string[] | null = null;
    const unsub = a.presence.subscribe((peers) => {
      latest = peers.map((p) => `${p.user.id}:${p.selection.join(",")}`);
    });

    b.presence.setSelection(["uuid-1"]);
    b.presence.setCursor({ x: 5, y: 7 });
    await settle();

    expect(latest).toEqual(["bob:uuid-1"]);
    const bob = a.presence.peers()[0]!;
    // setCursor must not have clobbered the selection set just before it.
    expect(bob.selection).toEqual(["uuid-1"]);
    expect(bob.cursor).toEqual({ x: 5, y: 7 });
    expect(bob.user.color).toBe(colorForUser("bob"));
    unsub();
  });
});
