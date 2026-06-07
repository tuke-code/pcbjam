# site

Public-facing marketing/content site for the KiCad WebAssembly project: landing
page, blog, and legal pages. Built with **Astro 6**.

This is a **standalone** project — it is intentionally decoupled from the `/web`
app monorepo (which is the product itself: React frontend + Fastify backend).
It uses **npm** (not the monorepo's pnpm) and has its own `package-lock.json`.

## What it ships

- Static by default: every page is prerendered to HTML and ships **zero client
  JavaScript**. A visitor downloads HTML + CSS only — no React/JS bundle.
- SSR-capable: the Vercel adapter is wired in, so any individual route can be
  switched to per-request server rendering without ripping anything out (see
  below).

## Routes

| Route                 | Source                                  |
| --------------------- | --------------------------------------- |
| `/`                   | `src/pages/index.astro`                 |
| `/blog`               | `src/pages/blog/index.astro`            |
| `/blog/<id>`          | `src/pages/blog/[slug].astro`           |
| `/terms`              | `src/pages/terms.astro`                 |
| `/privacy`            | `src/pages/privacy.astro`               |
| `/cookies`            | `src/pages/cookies.astro`               |

Blog posts are Markdown files in `src/content/blog/`, validated by the schema in
`src/content.config.ts`. Add a post by dropping a new `.md` file there with
`title`, `description`, and `pubDate` frontmatter.

## Local development

Requires **Node ≥ 22.12** (Astro 6 requirement).

```bash
cd site
npm install
npm run dev        # http://localhost:4321
npm run build      # outputs to dist/ (+ .vercel/output for the adapter)
npm run preview    # serve the production build locally
```

## SSR per route

Pages are static by default. To render a specific page or endpoint on demand
(per request, as a Vercel Function), add this to its frontmatter:

```astro
---
export const prerender = false;
---
```

That's the only change needed — the `@astrojs/vercel` adapter in
`astro.config.mjs` already provides the server runtime. The rest of the site
stays static.

## Deploying to Vercel

The `@astrojs/vercel` adapter emits the Vercel Build Output API format, so **no
`vercel.json` is required**.

1. Push this repo to GitHub.
2. Vercel dashboard → **New Project** → import this repo.
3. Click **Edit** next to **Root Directory** and set it to **`site`**. This is
   the standard way to deploy a project that lives in a subdirectory.
4. Vercel auto-detects the **Astro** framework preset and the package manager
   from the lockfile. Leave the build/install commands at their defaults.
5. Deploy. Static pages are served from the CDN; any route with
   `prerender = false` is deployed as a Vercel Function automatically.

Note: do **not** use `vercel.json` for URL rewrites with Astro — use Astro's
`redirects` option in `astro.config.mjs` instead.
