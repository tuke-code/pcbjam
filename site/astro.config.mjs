// @ts-check
import { defineConfig } from 'astro/config';
import vercel from '@astrojs/vercel';

// Static by default (every page prerenders to HTML, zero client JS).
// The Vercel adapter is wired in so any single route can opt into
// per-request SSR later with `export const prerender = false;`.
// See README.md ("SSR per route") for how.
export default defineConfig({
  output: 'static',
  adapter: vercel(),
  // Prefetch linked pages so SPA-style navigation feels instant.
  prefetch: { prefetchAll: true, defaultStrategy: 'viewport' },
});
