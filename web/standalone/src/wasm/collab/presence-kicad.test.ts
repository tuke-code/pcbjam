import { describe, expect, it, vi } from "vitest";
import { colorForUser, type PresenceState } from "@pcbjam/shared";
import {
  bindKicadPresence,
  hasPresenceBridge,
  parseSelectionEmit,
  xselFromPeerState,
  type PresenceKicadWindow,
} from "./presence-kicad";
import type { PresenceHandle, PresencePeer } from "./presence";
import type { CrossAppHandle, CrossAppPeer } from "./cross-app";

/**
 * presence-kicad bridge unit tests (collab-presence 0002): a fake Module +
 * window stand in for the wasm side; a stub presence handle stands in for
 * awareness. Verifies both directions and teardown.
 */

function fakeModule() {
  return {
    kicadCollabPresenceStart: vi.fn(),
    kicadCollabSetRemote: vi.fn(),
    kicadCollabGetViewport: vi.fn(() => '{"cx":0,"cy":0,"scale":1,"w":800,"h":600}'),
    kicadCollabGetSelection: vi.fn(() => "[]"),
  };
}

function stubPresence(peers: PresencePeer[] = []) {
  const subscribers = new Set<(p: PresencePeer[]) => void>();
  const handle: PresenceHandle & { firePeers(p: PresencePeer[]): void } = {
    peers: () => peers,
    subscribe(cb) {
      subscribers.add(cb);
      return () => subscribers.delete(cb);
    },
    setCursor: vi.fn(),
    setSelection: vi.fn(),
    colorOf: vi.fn((id: string) => colorForUser(id)),
    destroy: vi.fn(),
    firePeers(p) {
      peers = p;
      for (const cb of subscribers) cb(p);
    },
  };
  return handle;
}

function peer(id: string, over: Partial<PresencePeer> = {}): PresencePeer {
  return {
    clientId: 1,
    user: { id, name: id, color: colorForUser(id) },
    tool: "pcbnew",
    cursor: null,
    selection: [],
    updatedAt: 1,
    ...over,
  };
}

describe("bindKicadPresence", () => {
  it("routes C++ emits into the presence setters", () => {
    const mod = fakeModule();
    const win: PresenceKicadWindow = {};
    const presence = stubPresence();
    bindKicadPresence({ mod, win, presence });

    expect(mod.kicadCollabPresenceStart).toHaveBeenCalled();

    win.kicadCollab!.onSelection!('["uuid-1","uuid-2"]');
    expect(presence.setSelection).toHaveBeenCalledWith(["uuid-1", "uuid-2"]);

    win.kicadCollab!.onCursor!(5, 7, 1);
    expect(presence.setCursor).toHaveBeenCalledWith({ x: 5, y: 7 });
    win.kicadCollab!.onCursor!(0, 0, 0);
    expect(presence.setCursor).toHaveBeenCalledWith(null);
  });

  it("seeds an existing selection from the wasm at attach", () => {
    const mod = fakeModule();
    mod.kicadCollabGetSelection.mockReturnValue('["pre-selected"]');
    const presence = stubPresence();
    bindKicadPresence({ mod, win: {}, presence });
    expect(presence.setSelection).toHaveBeenCalledWith(["pre-selected"]);
  });

  it("pushes a throttled remote snapshot on peers change", async () => {
    const mod = fakeModule();
    const presence = stubPresence();
    bindKicadPresence({ mod, win: {}, presence });
    expect(mod.kicadCollabSetRemote).toHaveBeenCalledTimes(1); // initial push

    presence.firePeers([
      peer("bob", { cursor: { x: 10, y: 20 }, selection: ["u1"] }),
    ]);
    presence.firePeers([
      peer("bob", { cursor: { x: 11, y: 21 }, selection: ["u1"] }),
    ]);
    await new Promise((r) => setTimeout(r, 60));

    // Two rapid changes coalesce into ONE trailing push carrying the latest state.
    expect(mod.kicadCollabSetRemote).toHaveBeenCalledTimes(2);
    const snapshot = JSON.parse(mod.kicadCollabSetRemote.mock.calls.at(-1)![0]);
    expect(snapshot.peers).toEqual([
      {
        id: "bob",
        name: "bob",
        color: colorForUser("bob"),
        cursor: { x: 11, y: 21 },
        selection: ["u1"],
      },
    ]);
  });

  it("destroy unhooks the window and clears the remote overlay", () => {
    const mod = fakeModule();
    const win: PresenceKicadWindow = {};
    const presence = stubPresence();
    const binding = bindKicadPresence({ mod, win, presence });

    binding.destroy();
    expect(win.kicadCollab?.onSelection).toBeUndefined();
    expect(win.kicadCollab?.onCursor).toBeUndefined();
    const last = JSON.parse(mod.kicadCollabSetRemote.mock.calls.at(-1)![0]);
    expect(last.peers).toEqual([]);
  });

  it("hasPresenceBridge gates on the 0002 exports", () => {
    expect(hasPresenceBridge(fakeModule())).toBe(true);
    expect(hasPresenceBridge({})).toBe(false);
    expect(hasPresenceBridge(undefined)).toBe(false);
  });
});

// ── cross-app selection (0006) ───────────────────────────────────────────────

function crossState(tool: string, over: Partial<PresenceState> = {}): PresenceState {
  return {
    user: { id: "bob", name: "bob", color: colorForUser("bob") },
    tool,
    cursor: null,
    selection: [],
    updatedAt: 1,
    ...over,
  };
}

function stubCrossApp(initial: CrossAppPeer[] = []) {
  let peers = initial;
  const subscribers = new Set<() => void>();
  const handle: CrossAppHandle & { firePeers(p: CrossAppPeer[]): void } = {
    setSelection: vi.fn(),
    peers: () => peers,
    subscribe(cb) {
      subscribers.add(cb);
      return () => subscribers.delete(cb);
    },
    destroy: vi.fn(),
    firePeers(p) {
      peers = p;
      for (const cb of subscribers) cb();
    },
  };
  return handle;
}

describe("parseSelectionEmit", () => {
  it("handles the bare array (eeschema / pre-0006) and object shapes", () => {
    expect(parseSelectionEmit('["u1","u2"]')).toEqual({ uuids: ["u1", "u2"] });
    expect(parseSelectionEmit('{"uuids":["u1"],"fpPaths":["/p1"]}')).toEqual({
      uuids: ["u1"],
      fpPaths: ["/p1"],
    });
    expect(parseSelectionEmit('{"uuids":["u1"]}')).toEqual({ uuids: ["u1"] });
  });

  it("drops non-string entries and returns null on malformed payloads", () => {
    expect(parseSelectionEmit('["u1",42]')).toEqual({ uuids: ["u1"] });
    expect(parseSelectionEmit("not json")).toBeNull();
    expect(parseSelectionEmit("42")).toBeNull();
  });
});

describe("xselFromPeerState", () => {
  it("maps a pcbnew peer's footprint paths to symbol uuids", () => {
    expect(
      xselFromPeerState(crossState("pcbnew", { selectionPaths: ["/sheet-1/sym-1", "/sym-2"] })),
    ).toEqual(["sym-1", "sym-2"]);
    // Board selection uuids alone (no paths) map to nothing in eeschema.
    expect(xselFromPeerState(crossState("pcbnew", { selection: ["board-uuid"] }))).toEqual([]);
  });

  it("passes an eeschema peer's symbol uuids through, and skips other tools", () => {
    expect(xselFromPeerState(crossState("eeschema", { selection: ["sym-1"] }))).toEqual(["sym-1"]);
    expect(xselFromPeerState(crossState("pl_editor", { selection: ["u1"] }))).toEqual([]);
  });
});

describe("bindKicadPresence × crossApp", () => {
  it("forwards selection emits (uuids + fpPaths) into the cross-app room", () => {
    const mod = fakeModule();
    const win: PresenceKicadWindow = {};
    const presence = stubPresence();
    const crossApp = stubCrossApp();
    bindKicadPresence({ mod, win, presence, crossApp });

    win.kicadCollab!.onSelection!('{"uuids":["u1"],"fpPaths":["/p1"]}');
    expect(presence.setSelection).toHaveBeenCalledWith(["u1"]);
    expect(crossApp.setSelection).toHaveBeenCalledWith(["u1"], ["/p1"]);

    win.kicadCollab!.onSelection!('["u2"]');
    expect(crossApp.setSelection).toHaveBeenCalledWith(["u2"], undefined);
  });

  it("seeds cross-app from kicadCollabGetSelectionFull when present", () => {
    const mod = {
      ...fakeModule(),
      kicadCollabGetSelectionFull: vi.fn(() => '{"uuids":["pre"],"fpPaths":["/pp"]}'),
    };
    const crossApp = stubCrossApp();
    bindKicadPresence({ mod, win: {}, presence: stubPresence(), crossApp });
    expect(crossApp.setSelection).toHaveBeenCalledWith(["pre"], ["/pp"]);
  });

  it("appends cross-app peers to the remote snapshot as xsel ghost entries", async () => {
    const mod = fakeModule();
    const presence = stubPresence();
    const crossApp = stubCrossApp();
    bindKicadPresence({ mod, win: {}, presence, crossApp });

    crossApp.firePeers([
      { clientId: 7, state: crossState("eeschema", { selection: ["sym-1"] }) },
      // Empty mapped selection → no snapshot entry.
      { clientId: 8, state: crossState("pcbnew", { selection: ["board-uuid"] }) },
    ]);
    await new Promise((r) => setTimeout(r, 60));

    const snapshot = JSON.parse(mod.kicadCollabSetRemote.mock.calls.at(-1)![0]);
    expect(snapshot.peers).toEqual([
      {
        id: "bob#x7",
        name: "bob · sch",
        color: colorForUser("bob"),
        cursor: null,
        selection: [],
        xsel: ["sym-1"],
      },
    ]);
  });
});
