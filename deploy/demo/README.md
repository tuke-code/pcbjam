# demo.pcbjam.com deploy runbook

The no-backend demo: the GPL standalone on **Cloudflare Pages** (`demo.pcbjam.com`),
its WASM + example projects on a versioned **R2 CDN** (`cdn.pcbjam.com`). The
design/spec docs live in the private `pcbjam-private` repo
(`docs/features/demo-deploy/`).

```
git tag vX.Y.Z  ──▶  .github/workflows/deploy-demo.yml
   1. snapshot current WASM → cdn/wasm/manifest-<tag>.json   (reuse prebuilt)
   2. publish example gallery → cdn/content/<tag>/
   3. build standalone (pinned to CDN + tag) → dist/
   4. wrangler pages deploy → demo.pcbjam.com

(separately, only when the WASM changes)  publish-wasm.yml (Ubicloud) / local seed
   build WASM → cdn/wasm/<tool>/<ver>/ + registry.json
```

Everything here lives in the GPL `pcbjam` repo (the demo *is* the GPL standalone;
no closed code), so `publish-wasm.yml` reuses the existing Ubicloud build infra.

## One-time setup (Cloudflare — needs your account)

1. `pcbjam.com` zone on Cloudflare; note the **account id**.
2. Public bucket: `wrangler r2 bucket create pcbjam-cdn`.
3. Custom domain `cdn.pcbjam.com` → the `pcbjam-cdn` bucket (R2 → Settings → Custom Domains).
4. **Transform Rule** on `cdn.pcbjam.com` (Rules → Transform Rules → Modify Response Header),
   "set static" on all requests:
   - `Cross-Origin-Resource-Policy: cross-origin`
   - `Access-Control-Allow-Origin: *`
   (R2 can't set CORP as object metadata; the demo page is COEP `require-corp`.)
5. Cloudflare Pages project `pcbjam-demo`; custom domain `demo.pcbjam.com`.
6. API token (Workers/Pages: Edit, R2: Edit) → this repo's GitHub secrets
   `CLOUDFLARE_API_TOKEN` + `CLOUDFLARE_ACCOUNT_ID`.

## First-time WASM seed (local — run from the pcbjam repo root; `output/` is already built)

```sh
wrangler login                       # or export CLOUDFLARE_API_TOKEN + _ACCOUNT_ID
node scripts/deploy/publish-wasm.mjs --tag kicad-9.0.1 \
  --src output --driver r2 --bucket pcbjam-cdn --remote
```

This uploads per-tool content-addressed folders + `registry.json`. Re-runs are
idempotent (unchanged tools are skipped). `--tag` names the folder of any tool
whose bytes changed; pick something readable (the KiCad version works well).

> Preview the exact upload offline first with `--driver local --out /tmp/cdn`
> (writes the bucket layout + a `_uploads.json` of every object's HTTP metadata).

## Deploying the demo

Tag a release — `git tag v1.2.3 && git push --tags` — and `deploy-demo.yml` runs.
Or trigger it manually (Actions → deploy-demo → Run, with a tag). It does **not**
rebuild WASM; it snapshots whatever `publish-wasm` last published, so returning
users don't re-download the (large) WASM on releases that didn't change it.

Roll a bad tool back without redeploying the app: edit
`cdn/wasm/manifest-<tag>.json` to point the tool at an older `<ver>` (still in the
bucket); it's served uncached, so the next load picks it up.

## When the WASM changes

Run `publish-wasm.yml` (Actions → publish-wasm → Run, with a `wasm_tag`) — it
builds on Ubicloud (sharing ci-ubicloud's deps cache) and publishes — or re-run
the local seed with a new `--tag`. Then deploy a release as usual; the snapshot
picks up the new versions.

## Add example projects to the gallery

Edit [`gallery.json`](gallery.json) (references source files in the repo; no
copies). Preview: `node scripts/deploy/publish-content.mjs --tag t --driver local
--out /tmp/cdn`.
