// @ts-check
import { defineConfig, envField } from 'astro/config';
import vercel from '@astrojs/vercel';
import mdx from '@astrojs/mdx';

// Static by default (every page prerenders to HTML, zero client JS).
// The Vercel adapter is wired in so any single route can opt into
// per-request SSR later with `export const prerender = false;`.
// See README.md ("SSR per route") for how.
export default defineConfig({
  output: 'static',
  adapter: vercel(),
  // MDX lets the Gerber-viewer blog post embed the <GerberDemo /> component
  // inline (markdown posts can't import components).
  integrations: [mdx()],
  // Prefetch linked pages so SPA-style navigation feels instant.
  prefetch: { prefetchAll: true, defaultStrategy: 'viewport' },
  // Dev-server cross-origin isolation so the embedded Gerber viewer's WASM
  // threads (SharedArrayBuffer) work under `npm run dev`. Production headers are
  // scoped per-route in vercel.json. require-corp (not credentialless) for the
  // widest browser support incl. Safari 15.2+; safe because the site loads only
  // same-origin subresources.
  vite: {
    server: {
      headers: {
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'require-corp',
      },
    },
  },
  // Typed, validated server secrets for the waitlist endpoint. All optional so
  // the build never requires them and the endpoint degrades gracefully when a
  // key is absent (see src/pages/api/waitlist.ts). The @astrojs/vercel adapter
  // reads these from process.env at runtime — never inlined into the bundle.
  env: {
    schema: {
      RESEND_API_KEY: envField.string({ context: 'server', access: 'secret', optional: true }),
      RESEND_SEGMENT_ID: envField.string({ context: 'server', access: 'secret', optional: true }),
      WAITLIST_FROM_EMAIL: envField.string({
        context: 'server',
        access: 'secret',
        optional: true,
        default: 'PCBJam <hello@pcbjam.com>',
      }),
      // Comma-separated origins allowed to cross-post the waitlist form (e.g. the
      // static demo at demo.pcbjam.com, which has no backend of its own). Not a
      // secret — just config. Same-origin submits never hit this.
      WAITLIST_ALLOWED_ORIGINS: envField.string({
        context: 'server',
        access: 'public',
        optional: true,
        default: 'https://demo.pcbjam.com',
      }),
    },
  },
});
