# Gerber viewer blog demo

KiCad's `gerbview` compiled to WebAssembly, embedded (lazily) in the blog post.
Click → it streams the WASM and renders the bundled tiny_tapeout board in-browser.

## Files here (`site/public/gerber-demo/`)

| Path | What it does |
|------|--------------|
| `index.html` | The iframe target — minimal page with the `#main-window` / `#window-container` the WASM needs. |
| `boot.js` | Boot harness: configures Emscripten `Module`, seeds KiCad config, preloads the board into MEMFS, auto-opens it via `Module.arguments`, and injects `wx.js → wx-dom.js → gerbview.js`. Holds `R2_BASE` + the dev/prod asset-base switch. |
| `board/` | The tiny_tapeout Gerber layers (committed) the demo opens. |
| `poster.png` | Static fallback shown to browsers that can't run the live viewer. |
| `wasm/` | `wx.js`/`wx-dom.js`/`gerbview.js` (committed, served same-origin) + `gerbview.wasm`/`kicad-resources.bin` (git-ignored; served from R2 in prod, local in dev). Synced from `/output`. |

## Related pieces (elsewhere in `site/`)

| Path | What it does |
|------|--------------|
| `src/components/GerberDemo.astro` | The embed: lazy click-to-load iframe, cross-origin-isolation reload guard, feature-detect + poster fallback. |
| `src/content/blog/porting-kicad-graphics-to-webgl-with-claude.mdx` | The post that renders `<GerberDemo />`. |
| `astro.config.mjs` + `src/middleware.ts` | Dev cross-origin-isolation headers (COOP/COEP `require-corp`). |
| `vercel.json` | Prod COOP/COEP, scoped to the post + `/gerber-demo/` routes. |
| `scripts/sync-demo-wasm.sh` | Copy fresh WASM from `/output` into `wasm/` (run after a gerbview rebuild). |
| `scripts/r2-deploy.sh` + `scripts/r2-cors.json` | Upload the heavy binaries to Cloudflare R2 (`pcbjam-assets` → `assets.pcbjam.com`) and set CORS. |

## Run / update

```bash
# local dev (serves wasm/ from the local mirror)
scripts/sync-demo-wasm.sh && npm run dev   # /blog/porting-kicad-graphics-to-webgl-with-claude

# after a new gerbview build: refresh local mirror, then push binaries to R2
scripts/sync-demo-wasm.sh && scripts/r2-deploy.sh
```

The live viewer needs SharedArrayBuffer + WebGL2 (Chrome/Edge/Firefox, Safari 15.2+);
other browsers get `poster.png`.