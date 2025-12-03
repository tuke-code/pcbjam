The goal is to build kicad with wasm and run it in a browser
The original research docs are in /docs
A lot of native module have to be compiled to wasm, the most complex is wxwidgets
/kicad and /wxwidgets are git submodules from our own forks
The e2e tests are in /tests, with a README and WHATWORKS md files
The e2e tests are separated per feature
The tests depend on canvas, there's an app to find button positions, use that, don't find buttons by estimating pixels
The test have screenshots that are tracked with git, use compare-screenshots.sh to see what changed, update them when a new image is added

Our current goal is to test every wxwidgets feature kicad uses, write the wasm layer and e2e tests, documented in WHATWORKS
Never run builds manually, we have scripts that run the builds in the /scripts folder
Build wxwidgets and tests with scripts, not manually
Don't change the wxwidgets core unless absolutely necessary, try to fix things in the wasm layer
Don't try to guess what's broken in wxwidgets, use debug tools / symbols, supported by the build script

# Next up
porting kicad, with most of the dependencies shimmed out and load it in a browser
