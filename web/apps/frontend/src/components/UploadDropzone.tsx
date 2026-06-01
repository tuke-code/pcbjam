import * as React from "react";
import { useQueryClient } from "@tanstack/react-query";
import { Files, FolderUp, Loader2, Package } from "lucide-react";
import { uploadFiles, uploadZip, type UploadItem } from "@/lib/api";
import { Button } from "@/components/ui/button";

export function UploadDropzone({ slug }: { slug: string }) {
  const qc = useQueryClient();
  const [busy, setBusy] = React.useState(false);
  const [error, setError] = React.useState<string | null>(null);
  const filesRef = React.useRef<HTMLInputElement>(null);
  const folderRef = React.useRef<HTMLInputElement>(null);
  const zipRef = React.useRef<HTMLInputElement>(null);

  // `webkitdirectory` isn't in the React input typings; set it imperatively.
  React.useEffect(() => {
    if (folderRef.current) {
      folderRef.current.setAttribute("webkitdirectory", "");
      folderRef.current.setAttribute("directory", "");
    }
  }, []);

  const refresh = () => qc.invalidateQueries({ queryKey: ["project", slug] });

  const run = async (fn: () => Promise<unknown>) => {
    setBusy(true);
    setError(null);
    try {
      await fn();
      await refresh();
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err));
    } finally {
      setBusy(false);
    }
  };

  const onFiles = (e: React.ChangeEvent<HTMLInputElement>) => {
    const list = e.target.files;
    if (!list || list.length === 0) return;
    const items: UploadItem[] = Array.from(list).map((file) => ({
      // folder picker → webkitRelativePath; multi-file picker → name
      path:
        (file as File & { webkitRelativePath?: string }).webkitRelativePath ||
        file.name,
      file,
    }));
    void run(() => uploadFiles(slug, items));
    e.target.value = "";
  };

  const onZip = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    void run(() => uploadZip(slug, file));
    e.target.value = "";
  };

  return (
    <div className="rounded-lg border border-dashed p-6">
      <div className="flex flex-wrap items-center gap-3">
        <Button
          variant="outline"
          disabled={busy}
          onClick={() => filesRef.current?.click()}
        >
          <Files /> Upload files
        </Button>
        <Button
          variant="outline"
          disabled={busy}
          onClick={() => folderRef.current?.click()}
        >
          <FolderUp /> Upload folder
        </Button>
        <Button
          variant="outline"
          disabled={busy}
          onClick={() => zipRef.current?.click()}
        >
          <Package /> Upload .zip
        </Button>
        {busy && (
          <span className="flex items-center gap-2 text-sm text-muted-foreground">
            <Loader2 className="animate-spin" /> uploading…
          </span>
        )}
      </div>
      {error && <p className="mt-3 text-sm text-destructive">{error}</p>}

      <input
        ref={filesRef}
        type="file"
        multiple
        hidden
        onChange={onFiles}
      />
      <input ref={folderRef} type="file" multiple hidden onChange={onFiles} />
      <input
        ref={zipRef}
        type="file"
        accept=".zip,application/zip"
        hidden
        onChange={onZip}
      />
    </div>
  );
}
