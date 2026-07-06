import * as React from "react";
import { PRESENCE_COLORS } from "@pcbjam/shared";
import { Palette, X } from "lucide-react";

/**
 * DEV-TIME presence style tuner (VITE_PRESENCE_TUNER=1): live-patches the wasm
 * overlay style via kicadCollabSetStyle so we can try shapes, widths, alphas,
 * label placement, palettes etc. and pick the shipped defaults. Settings
 * persist in localStorage across reloads; "Copy JSON" exports the current
 * values to wire into collab_presence_style.h. "Demo peers/pins" injects
 * synthetic remote state (kicadCollabSetRemote/SetPins) so a SOLO tab can
 * preview everything — note a real awareness/comment change overwrites the
 * synthetic snapshot (re-toggle to restore).
 */

export interface TunerModule {
  kicadCollabSetStyle(json: string): void;
  kicadCollabSetRemote(json: string): void;
  kicadCollabSetPins(json: string): void;
  kicadCollabGetViewport(): string;
  kicadCollabTestListItems(n: number): string;
}

export function hasTunerBridge(mod: unknown): mod is TunerModule {
  const m = mod as Partial<TunerModule> | undefined;
  return (
    typeof m?.kicadCollabSetStyle === "function" &&
    typeof m?.kicadCollabTestListItems === "function"
  );
}

/** Mirror of collab_presence_style.h STYLE — defaults MUST match the C++. */
const DEFAULT_STYLE = {
  selShape: 0,
  selStrokeWidth: 2.5,
  selStrokeAlpha: 0.9,
  selFillAlpha: 0,
  selPaddingPx: 4,
  selCornerPx: 8,
  labelShow: true,
  labelSizePx: 9,
  labelChip: false,
  labelVPos: 0,
  labelHPos: 0,
  labelInside: false,
  labelOffsetPx: 8,
  cursorShape: 0,
  cursorSizePx: 7,
  cursorWidthPx: 2,
  cursorAlpha: 0.9,
  cursorLabel: true,
  cursorLabelSizePx: 10,
  cursorLabelChip: false,
  fixedColor: "",
  palette: [] as string[],
  pinRadiusPx: 7,
  pinRingPx: 1.5,
  pinRingAlpha: 0.9,
  pinFillAlpha: 1,
  pinResolvedAlpha: 0.3,
};

type Style = typeof DEFAULT_STYLE;

const STORE_KEY = "pcbjam:presence-style";

function loadStored(): Style {
  try {
    const raw = localStorage.getItem(STORE_KEY);
    return raw ? { ...DEFAULT_STYLE, ...JSON.parse(raw) } : { ...DEFAULT_STYLE };
  } catch {
    return { ...DEFAULT_STYLE };
  }
}

const SEL_SHAPES = ["rectangle", "corner brackets", "underline", "rounded rect", "filled only"];
const CURSOR_SHAPES = ["cross", "pointer", "circle + dot"];
const VPOS = ["top", "bottom"];
const HPOS = ["start", "end", "center"];

export function PresenceTuner({ mod }: { mod: TunerModule }) {
  const [open, setOpen] = React.useState(true);
  const [style, setStyle] = React.useState<Style>(loadStored);
  const [demo, setDemo] = React.useState(false);
  const [copied, setCopied] = React.useState(false);

  // Push on mount (restores a stored style after reload) + on every change.
  React.useEffect(() => {
    mod.kicadCollabSetStyle(JSON.stringify(style));
    try {
      localStorage.setItem(STORE_KEY, JSON.stringify(style));
    } catch {
      /* private mode */
    }
  }, [mod, style]);

  const set = <K extends keyof Style>(k: K, v: Style[K]) =>
    setStyle((s) => ({ ...s, [k]: v }));

  const injectDemo = React.useCallback(
    (on: boolean) => {
      setDemo(on);
      if (!on) {
        mod.kicadCollabSetRemote(JSON.stringify({ peers: [] }));
        mod.kicadCollabSetPins(JSON.stringify({ pins: [] }));
        return;
      }
      try {
        const vp = JSON.parse(mod.kicadCollabGetViewport());
        const items = JSON.parse(mod.kicadCollabTestListItems(3)) as string[];
        const spanX = vp.w / vp.scale;
        const spanY = vp.h / vp.scale;
        const peers = [
          {
            id: "demo-bob",
            name: "bob",
            color: PRESENCE_COLORS[8],
            cursor: { x: vp.cx - spanX * 0.18, y: vp.cy - spanY * 0.12 },
            selection: items.slice(0, 1),
          },
          {
            id: "demo-carol",
            name: "carol",
            color: PRESENCE_COLORS[4],
            cursor: { x: vp.cx + spanX * 0.15, y: vp.cy + spanY * 0.16 },
            selection: items.slice(1, 3),
          },
        ];
        const pins = [
          {
            id: "demo-pin-1",
            name: "bob",
            x: vp.cx - spanX * 0.05,
            y: vp.cy + spanY * 0.2,
            color: PRESENCE_COLORS[8],
            resolved: false,
          },
          {
            id: "demo-pin-2",
            name: "dave",
            x: vp.cx + spanX * 0.22,
            y: vp.cy - spanY * 0.18,
            color: PRESENCE_COLORS[0],
            resolved: false,
          },
        ];
        mod.kicadCollabSetRemote(JSON.stringify({ peers }));
        mod.kicadCollabSetPins(JSON.stringify({ pins }));
      } catch {
        /* frame not up yet */
      }
    },
    [mod],
  );

  const copyJson = async () => {
    try {
      await navigator.clipboard.writeText(JSON.stringify(style, null, 2));
      setCopied(true);
      setTimeout(() => setCopied(false), 1500);
    } catch {
      /* clipboard unavailable */
    }
  };

  if (!open) {
    return (
      <button
        title="Presence style tuner"
        onClick={() => setOpen(true)}
        className="absolute left-3 top-12 z-40 flex h-8 w-8 items-center justify-center rounded-full bg-fuchsia-700 text-white shadow-lg"
      >
        <Palette size={15} />
      </button>
    );
  }

  return (
    <div className="absolute left-3 top-12 z-40 flex max-h-[80vh] w-72 flex-col overflow-hidden rounded-lg bg-black/90 text-white shadow-xl ring-1 ring-inset ring-fuchsia-400/40">
      <div className="flex items-center justify-between bg-fuchsia-900/60 px-3 py-2">
        <span className="flex items-center gap-1.5 text-xs font-semibold">
          <Palette size={13} /> Presence tuner <span className="text-white/50">(dev)</span>
        </span>
        <button onClick={() => setOpen(false)} className="text-white/70 hover:text-white">
          <X size={14} />
        </button>
      </div>

      <div className="flex gap-2 border-b border-white/10 px-3 py-2">
        <button
          onClick={() => injectDemo(!demo)}
          className={`rounded px-2 py-1 text-[11px] ring-1 ring-inset ring-white/25 ${
            demo ? "bg-fuchsia-600" : "hover:bg-white/10"
          }`}
        >
          {demo ? "Demo on" : "Demo peers"}
        </button>
        <button
          onClick={copyJson}
          className="rounded px-2 py-1 text-[11px] ring-1 ring-inset ring-white/25 hover:bg-white/10"
        >
          {copied ? "Copied ✓" : "Copy JSON"}
        </button>
        <button
          onClick={() => {
            setStyle({ ...DEFAULT_STYLE });
            localStorage.removeItem(STORE_KEY);
          }}
          className="rounded px-2 py-1 text-[11px] ring-1 ring-inset ring-white/25 hover:bg-white/10"
        >
          Reset
        </button>
      </div>

      <div className="overflow-y-auto px-3 pb-3 text-[11px]">
        <Section title="Selection box">
          <Select label="shape" value={style.selShape} options={SEL_SHAPES} onChange={(v) => set("selShape", v)} />
          <Range label="border px" v={style.selStrokeWidth} min={0.5} max={8} step={0.5} onChange={(v) => set("selStrokeWidth", v)} />
          <Range label="border α" v={style.selStrokeAlpha} min={0.1} max={1} step={0.05} onChange={(v) => set("selStrokeAlpha", v)} />
          <Range label="infill α" v={style.selFillAlpha} min={0} max={0.6} step={0.02} onChange={(v) => set("selFillAlpha", v)} />
          <Range label="padding px" v={style.selPaddingPx} min={0} max={20} step={1} onChange={(v) => set("selPaddingPx", v)} />
          <Range label="corner px" v={style.selCornerPx} min={2} max={24} step={1} onChange={(v) => set("selCornerPx", v)} />
        </Section>

        <Section title="Name tag">
          <Check label="show" v={style.labelShow} onChange={(v) => set("labelShow", v)} />
          <Range label="size px" v={style.labelSizePx} min={5} max={20} step={0.5} onChange={(v) => set("labelSizePx", v)} />
          <Check label="chip background" v={style.labelChip} onChange={(v) => set("labelChip", v)} />
          <Select label="v-pos" value={style.labelVPos} options={VPOS} onChange={(v) => set("labelVPos", v)} />
          <Select label="h-pos" value={style.labelHPos} options={HPOS} onChange={(v) => set("labelHPos", v)} />
          <Check label="inside box" v={style.labelInside} onChange={(v) => set("labelInside", v)} />
          <Range label="offset px" v={style.labelOffsetPx} min={0} max={24} step={1} onChange={(v) => set("labelOffsetPx", v)} />
        </Section>

        <Section title="Cursor">
          <Select label="shape" value={style.cursorShape} options={CURSOR_SHAPES} onChange={(v) => set("cursorShape", v)} />
          <Range label="size px" v={style.cursorSizePx} min={3} max={20} step={0.5} onChange={(v) => set("cursorSizePx", v)} />
          <Range label="line px" v={style.cursorWidthPx} min={0.5} max={6} step={0.25} onChange={(v) => set("cursorWidthPx", v)} />
          <Range label="alpha" v={style.cursorAlpha} min={0.2} max={1} step={0.05} onChange={(v) => set("cursorAlpha", v)} />
          <Check label="name label" v={style.cursorLabel} onChange={(v) => set("cursorLabel", v)} />
          <Range label="label px" v={style.cursorLabelSizePx} min={5} max={20} step={0.5} onChange={(v) => set("cursorLabelSizePx", v)} />
          <Check label="label chip" v={style.cursorLabelChip} onChange={(v) => set("cursorLabelChip", v)} />
        </Section>

        <Section title="Colors">
          <div className="mb-1 flex items-center gap-2">
            <span className="w-24 text-white/60">fixed color</span>
            <input
              type="color"
              value={style.fixedColor || "#3b82f6"}
              onChange={(e) => set("fixedColor", e.target.value)}
              className="h-5 w-8"
            />
            <button
              onClick={() => set("fixedColor", "")}
              className="rounded px-1.5 ring-1 ring-inset ring-white/25 hover:bg-white/10"
            >
              off
            </button>
            <span className="text-white/40">{style.fixedColor || "per-user"}</span>
          </div>
          <div className="mb-1">
            <span className="mr-2 text-white/60">palette override (hex, comma-sep)</span>
            <textarea
              value={style.palette.join(",")}
              onChange={(e) =>
                set(
                  "palette",
                  e.target.value
                    .split(",")
                    .map((s) => s.trim())
                    .filter((s) => /^#[0-9a-fA-F]{6}$/.test(s)),
                )
              }
              placeholder="#ef4444,#22c55e,…  (empty = shipped palette)"
              className="mt-1 h-10 w-full resize-none rounded bg-white/10 p-1 text-[10px] text-white outline-none"
            />
            <div className="mt-1 flex gap-1">
              {(style.palette.length ? style.palette : [...PRESENCE_COLORS]).map((c, i) => (
                <span key={i} className="h-3 w-3 rounded-sm" style={{ background: c }} />
              ))}
            </div>
          </div>
        </Section>

        <Section title="Comment pins">
          <Range label="radius px" v={style.pinRadiusPx} min={3} max={16} step={0.5} onChange={(v) => set("pinRadiusPx", v)} />
          <Range label="ring px" v={style.pinRingPx} min={0} max={5} step={0.25} onChange={(v) => set("pinRingPx", v)} />
          <Range label="ring α" v={style.pinRingAlpha} min={0} max={1} step={0.05} onChange={(v) => set("pinRingAlpha", v)} />
          <Range label="fill α" v={style.pinFillAlpha} min={0.2} max={1} step={0.05} onChange={(v) => set("pinFillAlpha", v)} />
        </Section>
      </div>
    </div>
  );
}

function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <details open className="mt-2">
      <summary className="cursor-pointer text-xs font-semibold text-fuchsia-300">{title}</summary>
      <div className="mt-1 space-y-1">{children}</div>
    </details>
  );
}

function Range({
  label,
  v,
  min,
  max,
  step,
  onChange,
}: {
  label: string;
  v: number;
  min: number;
  max: number;
  step: number;
  onChange: (v: number) => void;
}) {
  return (
    <label className="flex items-center gap-2">
      <span className="w-24 shrink-0 text-white/60">{label}</span>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={v}
        onChange={(e) => onChange(Number(e.target.value))}
        className="min-w-0 flex-1"
      />
      <span className="w-9 text-right text-white/80">{v}</span>
    </label>
  );
}

function Check({ label, v, onChange }: { label: string; v: boolean; onChange: (v: boolean) => void }) {
  return (
    <label className="flex items-center gap-2">
      <span className="w-24 shrink-0 text-white/60">{label}</span>
      <input type="checkbox" checked={v} onChange={(e) => onChange(e.target.checked)} />
    </label>
  );
}

function Select({
  label,
  value,
  options,
  onChange,
}: {
  label: string;
  value: number;
  options: string[];
  onChange: (v: number) => void;
}) {
  return (
    <label className="flex items-center gap-2">
      <span className="w-24 shrink-0 text-white/60">{label}</span>
      <select
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        className="flex-1 rounded bg-white/10 px-1 py-0.5 text-white"
      >
        {options.map((o, i) => (
          <option key={o} value={i} className="bg-neutral-900">
            {o}
          </option>
        ))}
      </select>
    </label>
  );
}
