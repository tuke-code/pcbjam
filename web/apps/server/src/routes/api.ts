import { contract } from "@kicad-web/contract";
import { initServer } from "@ts-rest/fastify";
import * as svc from "../services/projects.js";

const s = initServer();

export const apiRouter = s.router(contract, {
  listProjects: async () => ({
    status: 200,
    body: await svc.listProjects(),
  }),

  createProject: async ({ body }) => {
    try {
      const project = await svc.createProject(body.name, body.slug);
      return { status: 201 as const, body: project };
    } catch (err) {
      if (err instanceof svc.SlugConflictError) {
        return { status: 409 as const, body: { message: err.message } };
      }
      throw err;
    }
  },

  getProject: async ({ params }) => {
    const result = await svc.getProjectWithFiles(params.project);
    if (!result) {
      return { status: 404 as const, body: { message: "project not found" } };
    }
    return { status: 200 as const, body: result };
  },

  deleteProject: async ({ params }) => {
    const id = await svc.deleteProject(params.project);
    if (!id) {
      return { status: 404 as const, body: { message: "project not found" } };
    }
    return { status: 200 as const, body: { id } };
  },

  listFiles: async ({ params }) => {
    const row = await svc.getProjectRowBySlug(params.project);
    if (!row) {
      return { status: 404 as const, body: { message: "project not found" } };
    }
    return { status: 200 as const, body: await svc.listFilesApi(row.id) };
  },
});

/** Fastify plugin that mounts the ts-rest JSON API. */
export const apiPlugin = s.plugin(apiRouter);
