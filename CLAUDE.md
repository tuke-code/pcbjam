The goal is to build kicad with wasm and run it in a browser
README.md details how to run the project

A lot of native module have to be compiled to wasm, the most complex is wxwidgets
/kicad and /wxwidgets are git submodules from our own forks
The e2e tests are in /tests, with a README and WHATWORKS md files
The e2e tests are separated per feature
Wxwidgets wasm port has hooks for finding positions of UI elements, tests use that
The test have screenshots that are tracked with git, use compare-screenshots.sh to see what changed
Update test images with /scripts/update-baseline-screenshots.sh when a new image is added
The tests have log files in tests/logs/{wxwidgets/kicad}/{test-name} after each run where the js console and cpp logs are visible
Always check screenshots for validating tests
Run e2e tests from /tests folder: `npm run test:kicad` or `npm run test:e2e` (not playwright directly)
Always run e2e specs in ALL THREE browsers — Firefox, Chrome, AND Safari (WebKit) — not just one; a spec must be green in all three engines. Each playwright config has firefox/chromium/webkit projects (e.g. `npm run test:asyncify:all`; chromium must be `--headed` on ARM Mac).

Build kicad with docker/build.sh (includes wxwidgets build, runs in docker)
Build wxwidgets standalone with scripts/build-wx-wasm.sh (runs on machine, for wxwidgets-only changes)
Build CPP wxwidgets tests with scripts/builds-wasm-test.sh
The build scripts pipe their outputs into log files so that they won't clog the LLM context. 
Don't pipe outputs, just run the scripts. Maybe with flex if you need that.

Don't change the wxwidgets core unless absolutely necessary, try to fix things in the wasm layer.
Don't change kicad unless absolutely necessary - keep our fork as close to upstream as possible.
Run scripts/kicad-diff-stats.sh to see how far our KiCad fork has diverged from upstream.
It's okay to add temporary logging that will be removed for debugging.

Don't try to guess what's broken , use debug tools / symbols, supported by the build scripts

Feature docs/patches are in features/<branch-name>/. Run scripts/create-feature-patches.sh to save patches for root, kicad, wxwidgets submodules.

The landing page / website is in /site (Astro, deployed as static assets to Cloudflare R2). When releasing a new version, bump the hardcoded build SHA `BUILD_SHA` in site/src/components/Footer.astro to the deployed main-repo commit. It's shown in the footer and links to that commit; because it pins the kicad + wxwidgets submodule revisions implicitly, it is our GPLv3 corresponding-source pointer (see /licenses). The site is static so there's no build-time git/env to set it automatically — it must be updated by hand each release.
