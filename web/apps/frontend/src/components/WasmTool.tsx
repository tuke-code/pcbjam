import * as React from "react";
import type { ProjectFile, Tool } from "@kicad-web/contract";
import { ChevronDown, ChevronUp } from "lucide-react";
import { fetchFileBytes } from "@/lib/api";
import { WASM_ASSET_BASE_URL } from "@/lib/config";
import { driveProjectIntoTool, hookIframeConsole } from "@/wasm/kicad-runner";

/**
 * Boots a KiCad tool by loading the proven harness HTML (/wasm/<tool>.html, the
 * same file the e2e tests use) in a same-origin iframe, then injects the project
 * tree into its MEMFS and drives File→Open. Same-origin is required: KiCad WASM
 * refuses to load its glue/wasm from a different origin under COEP.
 */
export function WasmTool({
  tool,
  slug,
  files,
  targetPath,
}: {
  tool: Tool;
  slug: string;
  files: ProjectFile[];
  targetPath?: string;
}) {
  const iframeRef = React.useRef<HTMLIFrameElement>(null);
  const startedRef = React.useRef(false);
  const [status, setStatus] = React.useState("Loading tool…");
  const [logs, setLogs] = React.useState<string[]>([]);
  const [showLog, setShowLog] = React.useState(false);

  const base = WASM_ASSET_BASE_URL.replace(/\/$/, "");
  const src = `${base}/${tool}.html`;

  const onLoad = () => {
    if (startedRef.current) return;
    startedRef.current = true;
    const win = iframeRef.current?.contentWindow as
      | ToolWindow
      | null
      | undefined;
    if (!win) {
      setStatus("Error: iframe has no window");
      return;
    }
    const append = (msg: string) =>
      setLogs((prev) => [...prev.slice(-800), msg]);
    hookIframeConsole(win, append);

    void driveProjectIntoTool(win, {
      tool,
      slug,
      files,
      targetPath,
      fetchBytes: (relPath) => fetchFileBytes(slug, relPath),
      log: append,
      onStatus: setStatus,
    }).catch((err) => {
      append(`[fatal] ${String(err)}`);
      setStatus(`Error: ${String(err)}`);
    });
  };

  return (
    <div className="relative h-screen w-screen overflow-hidden bg-[#1a1a2e]">
      <iframe
        ref={iframeRef}
        src={src}
        title={`${tool} (${slug})`}
        onLoad={onLoad}
        className="absolute inset-0 h-full w-full border-0"
        allow="cross-origin-isolated; fullscreen"
      />

      {status && (
        <div className="pointer-events-none absolute left-3 top-3 z-20 rounded bg-black/70 px-3 py-2 font-mono text-xs text-white">
          {status}
        </div>
      )}

      <div className="absolute bottom-0 left-0 right-0 z-20">
        <button
          className="flex items-center gap-1 bg-black/70 px-3 py-1 font-mono text-xs text-white"
          onClick={() => setShowLog((s) => !s)}
        >
          {showLog ? <ChevronDown size={14} /> : <ChevronUp size={14} />} console
          ({logs.length})
        </button>
        {showLog && (
          <pre className="max-h-64 overflow-auto bg-black/85 p-3 font-mono text-[11px] leading-tight text-green-300">
            {logs.join("\n")}
          </pre>
        )}
      </div>
    </div>
  );
}
