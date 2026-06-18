import { contract } from "@pcbjam/shared";
import { initClient } from "@ts-rest/core";
import { API_BASE_URL } from "./config";

/**
 * ts-rest client over the shared contract (the REST backend). Shared by the
 * remote project source (lib/project-source.ts) and the lib/drift endpoints
 * (lib/api.ts). Kept in its own module so project-source and api don't import
 * each other (avoids a cycle).
 */
export const client = initClient(contract, {
  baseUrl: API_BASE_URL,
  baseHeaders: {},
});
