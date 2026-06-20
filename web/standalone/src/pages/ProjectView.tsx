import * as React from "react";
import { Link, useNavigate, useParams } from "react-router-dom";
import { useQueryClient } from "@tanstack/react-query";
import {
  EXTENSION_TOOL,
  FILELESS_TOOLS,
  TOOL_LABELS,
  type Tool,
} from "@pcbjam/shared";
import {
  ArrowLeft,
  Copy,
  Download,
  ExternalLink,
  FilePlus,
  Loader2,
  Trash2,
} from "lucide-react";
import { fetchFileBytes, useProject, useSourceDescriptor } from "@/lib/api";
import { downloadBytes } from "@/lib/download";
import { localProjectStore } from "@/lib/project-source";
import { DOCUMENT_TOOLS } from "@/lib/new-file";
import { zipFiles } from "@/lib/zip";
import { formatBytes } from "@/lib/utils";
import { Button } from "@/components/ui/button";
import { SourceChip } from "@/components/SourceChip";
import { NewFileDialog } from "@/components/NewFileDialog";

function toolForPath(path: string): Tool | null {
  const dot = path.lastIndexOf(".");
  if (dot < 0) return null;
  return EXTENSION_TOOL[path.slice(dot).toLowerCase()] ?? null;
}

/**
 * Project-centric view (mirrors the closed app): Files + a "New file" row whose
 * document-tool buttons create a templated file in THIS project and open it, so
 * you always edit a real file that saves back. Writable projects (local IDB or a
 * remote-rw backend) get New / export; a read-only gallery project gets "Save a
 * copy to browser" (fork into editable IDB). The source chip says where edits go.
 */
export function ProjectView() {
  const { project: slug = "" } = useParams();
  const navigate = useNavigate();
  const qc = useQueryClient();
  const { data, isLoading, error } = useProject(slug);
  const { data: descriptor } = useSourceDescriptor(slug);
  const isLocal = descriptor?.kind === "local";
  const canFork = descriptor?.kind === "remote-ro" && !!localProjectStore();
  const [busy, setBusy] = React.useState(false);
  const [newFileTool, setNewFileTool] = React.useState<Tool | null>(null);

  const exportZip = async () => {
    const store = localProjectStore();
    if (!store) return;
    setBusy(true);
    try {
      downloadBytes(`${slug}.zip`, zipFiles(await store.readFiles(slug)));
    } finally {
      setBusy(false);
    }
  };

  const downloadOne = async (path: string) => {
    downloadBytes(path, await fetchFileBytes(slug, path));
  };

  const remove = async () => {
    const store = localProjectStore();
    if (!store || !data) return;
    if (!window.confirm(`Delete "${data.project.name}" from this browser? This can't be undone.`))
      return;
    await store.deleteProject(slug);
    await qc.invalidateQueries({ queryKey: ["local-projects"] });
    navigate("/");
  };

  // Copy a read-only gallery project into an editable browser-local project.
  const fork = async () => {
    const store = localProjectStore();
    if (!store || !data) return;
    setBusy(true);
    try {
      const files = await Promise.all(
        data.files.map(async (f) => ({
          path: f.path,
          bytes: await fetchFileBytes(slug, f.path),
        })),
      );
      const created = await store.createProject(data.project.name, files);
      await qc.invalidateQueries({ queryKey: ["local-projects"] });
      window.location.assign(`/p/${created.slug}`);
    } catch {
      setBusy(false);
    }
  };

  return (
    <div className="container py-10">
      <Button asChild variant="ghost" size="sm" className="mb-4">
        <Link to="/">
          <ArrowLeft /> Home
        </Link>
      </Button>

      {isLoading && (
        <p className="flex items-center gap-2 text-muted-foreground">
          <Loader2 className="animate-spin" /> loading…
        </p>
      )}
      {error && <p className="text-destructive">{(error as Error).message}</p>}

      {data && (
        <>
          <div className="mb-6">
            <h1 className="flex flex-wrap items-center gap-3 text-2xl font-semibold tracking-tight">
              {data.project.name}
              {descriptor && <SourceChip descriptor={descriptor} />}
            </h1>
            <p className="text-sm text-muted-foreground">/p/{data.project.slug}</p>
          </div>

          {/* New file (writable projects): a templated doc, created then opened. */}
          {descriptor?.writable && (
            <div className="mb-6">
              <h2 className="mb-2 text-sm font-medium text-muted-foreground">
                New file
              </h2>
              <div className="flex flex-wrap gap-2">
                {DOCUMENT_TOOLS.map((tool) => (
                  <Button
                    key={tool}
                    variant="outline"
                    size="sm"
                    onClick={() => setNewFileTool(tool)}
                  >
                    <FilePlus size={14} /> {TOOL_LABELS[tool]}
                  </Button>
                ))}
              </div>
            </div>
          )}

          {/* Project-level actions. */}
          <div className="mb-6 flex flex-wrap gap-3">
            {canFork && (
              <Button variant="outline" size="sm" disabled={busy} onClick={() => void fork()}>
                {busy ? <Loader2 className="animate-spin" size={15} /> : <Copy size={15} />}
                Save a copy to browser
              </Button>
            )}
            {isLocal && (
              <>
                <Button variant="outline" size="sm" disabled={busy} onClick={() => void exportZip()}>
                  {busy ? <Loader2 className="animate-spin" size={15} /> : <Download size={15} />}
                  Download .zip
                </Button>
                <Button
                  variant="ghost"
                  size="sm"
                  className="text-destructive hover:text-destructive"
                  onClick={() => void remove()}
                >
                  <Trash2 size={15} /> Delete
                </Button>
              </>
            )}
          </div>

          {/* File-less tools (gerber viewer, calculator, lib editors) — still
              project-scoped, opened without a target file. Full reload. */}
          <div className="mb-6 flex flex-wrap gap-3">
            {[...FILELESS_TOOLS].map((tool) => (
              <a
                key={tool}
                className="text-sm underline underline-offset-4"
                href={`/p/${slug}/${tool}/`}
              >
                Open {TOOL_LABELS[tool]}
              </a>
            ))}
          </div>

          <h2 className="mb-3 text-lg font-medium">Files ({data.files.length})</h2>
          <div className="divide-y rounded-lg border">
            {data.files.map((f) => {
              const tool = toolForPath(f.path);
              return (
                <div
                  key={f.id}
                  className="flex items-center justify-between gap-4 px-4 py-2.5"
                >
                  <div className="min-w-0">
                    <p className="truncate font-mono text-sm">{f.path}</p>
                    <p className="text-xs text-muted-foreground">
                      {formatBytes(f.size)}
                    </p>
                  </div>
                  <div className="flex shrink-0 items-center gap-1">
                    {isLocal && (
                      <button
                        className="inline-flex items-center gap-1 rounded-md border px-2.5 py-1.5 text-sm hover:bg-accent"
                        title="Download this file"
                        onClick={() => void downloadOne(f.path)}
                      >
                        <Download size={14} />
                      </button>
                    )}
                    {tool && (
                      <a
                        className="inline-flex items-center gap-1 rounded-md border px-3 py-1.5 text-sm hover:bg-accent"
                        href={`/p/${slug}/${tool}/${f.path}`}
                      >
                        <ExternalLink size={14} /> Open in {TOOL_LABELS[tool]}
                      </a>
                    )}
                  </div>
                </div>
              );
            })}
            {data.files.length === 0 && (
              <div className="px-4 py-6 text-sm text-muted-foreground">
                No files yet — use “New file” above to create one.
              </div>
            )}
          </div>

          <NewFileDialog
            tool={newFileTool}
            onClose={() => setNewFileTool(null)}
            project={{ slug, existingPaths: data.files.map((f) => f.path) }}
          />
        </>
      )}
    </div>
  );
}
