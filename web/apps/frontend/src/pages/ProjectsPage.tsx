import * as React from "react";
import { Link } from "react-router-dom";
import { Loader2, Plus, Trash2 } from "lucide-react";
import { useCreateProject, useDeleteProject, useProjects } from "@/lib/api";
import { Button } from "@/components/ui/button";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";

function CreateProjectDialog() {
  const create = useCreateProject();
  const [open, setOpen] = React.useState(false);
  const [name, setName] = React.useState("");
  const [slug, setSlug] = React.useState("");

  const submit = async () => {
    if (!name.trim()) return;
    await create.mutateAsync({
      name: name.trim(),
      slug: slug.trim() || undefined,
    });
    setName("");
    setSlug("");
    setOpen(false);
  };

  return (
    <Dialog open={open} onOpenChange={setOpen}>
      <DialogTrigger asChild>
        <Button>
          <Plus /> New project
        </Button>
      </DialogTrigger>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Create a project</DialogTitle>
          <DialogDescription>
            A project holds a tree of KiCad files you can open in the browser.
          </DialogDescription>
        </DialogHeader>
        <div className="space-y-3">
          <div className="space-y-1.5">
            <Label htmlFor="name">Name</Label>
            <Input
              id="name"
              value={name}
              placeholder="My board"
              onChange={(e) => setName(e.target.value)}
            />
          </div>
          <div className="space-y-1.5">
            <Label htmlFor="slug">Slug (optional)</Label>
            <Input
              id="slug"
              value={slug}
              placeholder="auto-generated from name"
              onChange={(e) => setSlug(e.target.value)}
            />
          </div>
          {create.error && (
            <p className="text-sm text-destructive">
              {(create.error as Error).message}
            </p>
          )}
        </div>
        <DialogFooter>
          <Button onClick={submit} disabled={create.isPending}>
            {create.isPending && <Loader2 className="animate-spin" />} Create
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}

export function ProjectsPage() {
  const { data: projects, isLoading, error } = useProjects();
  const del = useDeleteProject();

  return (
    <div className="container py-10">
      <div className="mb-8 flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-semibold tracking-tight">Projects</h1>
          <p className="text-sm text-muted-foreground">
            Create a project, upload KiCad files, open them in the browser.
          </p>
        </div>
        <CreateProjectDialog />
      </div>

      {isLoading && (
        <p className="flex items-center gap-2 text-muted-foreground">
          <Loader2 className="animate-spin" /> loading…
        </p>
      )}
      {error && (
        <p className="text-destructive">
          Could not load projects: {(error as Error).message}
        </p>
      )}

      <div className="grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-3">
        {projects?.map((p) => (
          <Card key={p.id}>
            <CardHeader>
              <CardTitle>{p.name}</CardTitle>
              <CardDescription>/p/{p.slug}</CardDescription>
            </CardHeader>
            <CardContent className="flex items-center justify-between">
              <Button asChild variant="secondary" size="sm">
                <Link to={`/p/${p.slug}`}>Open</Link>
              </Button>
              <Button
                variant="ghost"
                size="icon"
                onClick={() => {
                  if (confirm(`Delete project "${p.name}"?`)) {
                    del.mutate(p.slug);
                  }
                }}
              >
                <Trash2 className="text-destructive" />
              </Button>
            </CardContent>
          </Card>
        ))}
      </div>

      {projects && projects.length === 0 && !isLoading && (
        <p className="text-muted-foreground">No projects yet. Create one above.</p>
      )}
    </div>
  );
}
