import { Link, useParams } from "react-router-dom";
import {
  EXTENSION_TOOL,
  FILELESS_TOOLS,
  TOOL_LABELS,
  type Tool,
} from "@kicad-web/contract";
import { ArrowLeft, ExternalLink, Loader2 } from "lucide-react";
import { useProject } from "@/lib/api";
import { formatBytes } from "@/lib/utils";
import { Button } from "@/components/ui/button";
import { UploadDropzone } from "@/components/UploadDropzone";

function toolForPath(path: string): Tool | null {
  const dot = path.lastIndexOf(".");
  if (dot < 0) return null;
  const ext = path.slice(dot).toLowerCase();
  return EXTENSION_TOOL[ext] ?? null;
}

export function ProjectDetailPage() {
  const { project: slug = "" } = useParams();
  const { data, isLoading, error } = useProject(slug);

  return (
    <div className="container py-10">
      <Button asChild variant="ghost" size="sm" className="mb-4">
        <Link to="/">
          <ArrowLeft /> All projects
        </Link>
      </Button>

      {isLoading && (
        <p className="flex items-center gap-2 text-muted-foreground">
          <Loader2 className="animate-spin" /> loading…
        </p>
      )}
      {error && (
        <p className="text-destructive">{(error as Error).message}</p>
      )}

      {data && (
        <>
          <div className="mb-6">
            <h1 className="text-2xl font-semibold tracking-tight">
              {data.project.name}
            </h1>
            <p className="text-sm text-muted-foreground">/p/{data.project.slug}</p>
          </div>

          <div className="mb-6 flex flex-wrap gap-3">
            {/* Standalone (file-less) tools — launched without a target file.
                Full reload (anchor) so Emscripten boots into a clean page. */}
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

          <div className="mb-8">
            <UploadDropzone slug={slug} />
          </div>

          <h2 className="mb-3 text-lg font-medium">
            Files ({data.files.length})
          </h2>
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
                  {tool && (
                    <a
                      className="inline-flex shrink-0 items-center gap-1 rounded-md border px-3 py-1.5 text-sm hover:bg-accent"
                      href={`/p/${slug}/${tool}/${f.path}`}
                    >
                      <ExternalLink size={14} /> Open in {TOOL_LABELS[tool]}
                    </a>
                  )}
                </div>
              );
            })}
            {data.files.length === 0 && (
              <div className="px-4 py-6 text-sm text-muted-foreground">
                No files yet — upload some above.
              </div>
            )}
          </div>
        </>
      )}
    </div>
  );
}
