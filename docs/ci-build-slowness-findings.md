# CI build slowness — root-cause findings (handoff)

> Why the Hetzner CI build takes ~2 h. The bottleneck is the host-side
> `wasm-opt -O2` pass (~76–88 min). **This doc was substantially revised** after
> pulling the per-pass `/usr/bin/time -v` counters out of the real CI runs and a
> multi-source deep-research pass: the earlier "irreducible 3.5 CPU-hours of
> work, environment only moves it ±15%" conclusion was **wrong**. See
> "Correction log" at the bottom for what changed and why.

## 🆕 RUN #4 VERDICT (CI run 27226030304, 2026-06-09): v130 works; the remaining 4h is ORCHESTRATION
Run #4 built **all 6 tools** on v130 on the ccx53 in **4h05m** (18:10→22:15 UTC).
The v121 lock convoy is confirmed dead on-box: pcbnew's `-O2` ran with **system
time 81 s / 1,495 voluntary ctx-switches** (v121: 114,075 s / 180 M). The 4h has
three *new*, measured causes — all orchestration, none of them wasm-opt pathology:

| Phase | Wall | Cause |
|---|---|---|
| setup | 4 min | fine |
| deps + wx + pcbnew compile | 50 min (18:12→19:02) | **docker-compose capped the container at `cpus: '10'`** (dev-Mac default) on the 32-core box, with `-j 32` oversubscribed on top |
| pcbnew asyncify 24 min + `-O2` 66 min | 90 min (19:02→20:32) | pcbnew is **338 MB** pre-O2 (eeschema: 188 MB). `-O2` = 13,993 s user @ **354% CPU** (BINARYEN_CORES=8) — *real* compute, Amdahl-capped at ~4 effective cores. This is the irreducible critical path. |
| 5 remaining tools, strictly sequential | 103 min (20:32→22:15) | each tool's host-side wasm-opt **blocked** the next tool's container compile; wasm-opt uses ~4 of 32 cores while the container idles |

eeschema's `-O2` was 10:38 — exactly the bench prediction, so the bench fixture
generalizes. The Mac does pcbnew's asyncify+O2 in ~35 min @ 6 cores (arm64
per-core advantage); the CI gap beyond that is orchestration, fixed by:
1. **Lift the compose caps in CI** — `KICAD_DOCKER_CPUS`/`KICAD_DOCKER_MEM` env
   interpolation in docker-compose.yml; CI sets nproc/110G (was 10/32G).
2. **Pipeline host-side wasm-opt with the next tool's compile** —
   `KICAD_PIPELINE=1` in docker/build.sh backgrounds dyncall+finalize+asyncify+O2
   (max `KICAD_PIPELINE_JOBS=2` concurrent; pcbnew `-O2` peaks 33.6 GB RSS).
   CI-only: a 32 GB dev Mac can't stack two postprocesses.
3. **BINARYEN_CORES=16** in the validate workflow (bench: 32c=8:02 vs 8c≈10:00
   on the eeschema fixture — mild win, and two concurrent postprocesses share
   the box with the compile).
4. **Binaryen default bumped 121→130** in get-wasm-opt.sh (validated: local
   31/31 e2e; CI run #4 Chromium fully green).

Expected: ~2h–2h20m (floor = deps + pcbnew compile + pcbnew's 90-min wasm-opt
chain). Below that needs Lever E (shrink pcbnew's `-O2` work) and/or caching
deps across runs (the ephemeral runner rebuilds deps+wx every time, ~30–50 min).

**Run #4 e2e: 16 passed / 14 failed — ALL 14 Firefox-only**, across every tool
("GL canvas has zero dimensions", wizard never appears), while Chromium passed
everything. Headless-Firefox/WebGL environment problem on the Hetzner VM, not a
v130 regression (locally Firefox passes; and `-O2` exists for V8's locals limit,
i.e. Chromium is the engine that matters for the corruption check). Open issue,
tracked separately from build time.

Correction: the "all 6 tools rc=0 in ~1h04m" local claim below was wrong — the
actual log (`logs/build/20260609-191138.log`) spans 19:11→21:15 ≈ **2h04m**
(`-j 3`, BINARYEN_CORES=6). The conclusion (v130 builds a working KiCad) stands.

## ✅ FINAL VALIDATION (run 27280051992, 2026-06-10): all 6 tools in **1h14m41s** (was 4h05m, 3.3x)
Full cold `all` build on the ccx53 with everything adopted (compose caps lifted,
KICAD_PIPELINE=1, BINARYEN_CORES=16, self-built wasm-opt v130):
- pcbnew: asyncify **5:12** (was 24:07 — the self-built binary at pcbnew scale),
  `-O2` **52:09** (was 1:06:13). Its 58-min postprocess fully overlapped ALL
  five other tools' compiles + postprocesses (eeschema asyncify 1:00 — was 7:22).
- Critical path is now ≈ deps + pcbnew compile + pcbnew asyncify+`-O2` ≈ the
  whole 1h15. Going below ~1h10 requires shrinking pcbnew's `-O2` input —
  see the wasm-EH (-fwasm-exceptions / KICAD_WASM_EH) experiment.
- e2e: 16 passed / 14 failed — the failures are the SAME 14 Firefox-only
  environmental tests as run #4 (Chromium 100% green). No regression; Firefox
  headless-GL on the Hetzner VM remains a separate open issue.

## 🆕 2026-06-10 EXPERIMENT DAY: orchestration verified on-box + the release-tarball discovery

**Repro run 27273412419** (calculator,pl_editor, pipelined, no e2e, ccx53): all
orchestration fixes confirmed on the real runner — deps 14 min (was 28 at the
10-CPU cap), pipeline overlap engaged (calculator's wasm-opt ran during
pl_editor's -j32 compile), whole step **32 min**.

**The official Linux Binaryen release tarballs are badly built.** Measured on
identical fixtures with sha256-identical outputs:
- x86_64 (ccx53, run 27276830256, BINARYEN_CORES=16): asyncify 3:50 → **0:58
  (4x)** with a stock gcc -O3+LTO self-build; -O2 equal (4:22 vs 4:20).
- aarch64 (M-chip QEMU VM): asyncify **13x** faster self-built; clang -O3+LTO
  also beats the tarball's -O2 by 3.5%. With a good binary, Linux ≈ macOS on
  the same silicon (2:59 vs 2:53) — the "Linux is slow" gap was binary quality.
- macOS arm64: the official tarball is *well*-built (self-build 12% slower) —
  keep the tarball on dev Macs; self-build is a Linux-CI-only fix.
→ Adopted: `BINARYEN_BUILD_FROM_SOURCE=1` in get-wasm-opt.sh (one-time ~5-min
build per ephemeral runner, cached in build-wasm/tools, builds wasm-opt +
wasm-emscripten-finalize). Good upstream-issue material (WebAssembly/binaryen).

**Allocator: dead lever on v130** (VM sweep, calculator fixture): glibc 5:37,
jemalloc 5:47, mimalloc 6:59. The jemalloc preload is harmless legacy now.

**arm64 cloud runners: ruled out** (run 27273412432, ubicloud-standard-8-arm):
calculator asyncify 10:42 / -O2 8:40 — Ampere burns ~1.6x the EPYC cycles and
~2x the M-chip's on the Amdahl-bound wasm-opt. Per-core speed is what matters.

**Projection for `all`** with everything adopted: setup 3 + deps 14 + pcbnew
compile ~10 + binaryen build 5 + pcbnew asyncify ~6 + pcbnew -O2 ~60, other 5
tools fully overlapped ≈ **~1h35-40m** (from 4h05m). Next levers beyond that:
Lever E on pcbnew's -O2 (user ruled out -O1; pass-subset/removelist remain) and
deps caching across runners.

## ✅ THE FIX (bench run 27210317273): upgrade Binaryen 121 → 130
Measured on the cached 188 MB fixture, same `-O2`, identical output size:

| Binaryen | cores | `-O2` wall | system time | ctx-switches | effective cores |
|---|---|---|---|---|---|
| **121** (current) | 32 | **1:12:52** | 89% | 180,000,000 | ~2.6 |
| **130** | 32 | **8:02** | **1%** | **3,599** | **~10** |
| 130 | 8 | ~10:00 | 1% | 848 | ~5.8 |

**~9× faster.** v130 eliminates the `wasm::Type` lock convoy: system time 89%→1%,
context-switches 180M→3,599, and the work that was capped at ~2.6 parallel cores now
scales to ~10. This is the whole story — the "more cores = slower" and "weak Mac beats
strong Linux" symptoms were all downstream of the v121 contention bug, fixed by v130.
Ruled out on-box: `-O1` (still 88% system — lock is pass-independent) and fewer threads
(still ~4 effective cores — lock caps it regardless). It is the **version**, full stop.

**Remaining work = validation (run #3):** the speed win is solid, but it was measured
running v130's `-O2` on a *v121*-asyncified module. `get-wasm-opt.sh` warns that
Binaryen/emsdk skew can corrupt asyncify metadata, so the real change is to build the
**whole** asyncify+`-O2` step on v130 (now selectable via `BINARYEN_VERSION=130`) and
run the **Chrome e2e suite** to confirm the app still loads. If e2e passes, bump the
default in `get-wasm-opt.sh` (and check the emsdk-bundled Binaryen matches). If it hits
"func is not a function", the emsdk Binaryen also needs bumping.

## ✅ VALIDATED (local cold build, 2026-06-09): v130 builds a WORKING KiCad
Full from-source cold build of **all 6 tools** with `BINARYEN_VERSION=130` (no
artifacts/fixture reuse), then the KiCad e2e suite in Firefox + Chromium:

- **Build:** `BINARYEN_VERSION=130 ./docker/build.sh all --build-deps -j 3` → all 6
  tools rc=0 in ~1h04m. Each used the standalone **binaryen-130** wasm-opt for the
  asyncify+`-O2` step (confirmed in logs). pcbnew `-O2` shrank it 338 MB → 187 MB, so
  the optimizer ran correctly — **no asyncify-metadata corruption, no "func is not a
  function".** Sizes: pcbnew 187M, eeschema 99M, symbol_editor 99M, pl_editor 53M,
  gerbview 50M, calculator 38M.
- **kicad e2e (`npm run test:kicad`): 31 passed / 1 skipped / 0 failed** across
  Firefox + Chromium. Every tool renders and passes in-browser on v130.
- `-j 3` (not the default `-j 10`) is required **locally only** — Docker Desktop's
  15.6 GB VM OOM-kills the OpenCASCADE compile at `-j 10` on a fresh `--build-deps`.
  The 128 GB Hetzner CI box has no such limit and uses `-j $(nproc)`.

CI cross-check (run #4, validate workflow, `build all`) is the on-box confirmation.
NB: run #3 (eeschema-only build) showed 26 e2e failures — those were the 5 **unbuilt**
tools' missing wasm + Firefox flake, **not** a v130 regression: the identical eeschema
Firefox tests that failed there (`eeschema-ui` Delete/Backspace, text-tool dialog) all
**pass** in this full-build run. → Safe to bump the `get-wasm-opt.sh` default to 130.

## ⚠️ VERDICT (measured on-box, bench run 27197360957) — supersedes the memory theory
**The `-O2` cost is ~90% FUTEX LOCK CONTENTION inside wasm-opt, not memory
management.** `perf` on the live ccx53 shows ~92% of CPU in
`do_futex → _raw_spin_lock → native_queued_spin_lock_slowpath` — threads spinning
in the kernel on a contended lock — **identical under glibc (92%) and mimalloc
(90%)**. The lock is Binaryen's own global type mutex (`wasm::Type`), hit by every
worker thread; more threads → worse contention (a lock convoy). Hard evidence it is
**not** the allocator/THP/madvise theory below:
- `madvise` calls = **0** (perf syscall count). The "purge storm" does not exist here.
- THP `compact_stall` Δ = **0**, `thp_fault_alloc` Δ = **0**. No compaction. (THP=madvise mode.)
- `mimalloc-retain` (`MIMALLOC_PURGE_DELAY=-1`) vs baseline: **1:12:52 → 1:06:51,
  only 8% faster**, both ~88.7% system, both ~90% futex-spinlock. Allocator is irrelevant.

So **jemalloc / mimalloc / retain-configs / THP=never are all DEAD ENDS** (now proven
on-box, not just argued). The levers that can actually move wall-clock:
1. **Newer Binaryen** — the devs cut this exact `wasm::Type` contention after our
   pinned **v121** (latest is v130). Highest-value; coupled to the emsdk Binaryen,
   needs Chrome e2e. **The #1 thing to test.**
2. **Fewer threads** (`BINARYEN_CORES=4–8`) — fewer threads on the one lock = far
   less contention + far less wasted CPU. But `-O2` only does ~2.6 cores of *real*
   work at any cores (true at 8 and 32), so this is mostly an **efficiency/cost win,
   probably not a big wall-clock win** — the ~2.6× parallelism is the wall floor.
3. **Less `-O2` work** — the fixture is **188 MB** of asyncify bloat and `-O2` runs
   every pass over all of it (~11,500 CPU-s of real work = the wall floor). A lighter
   pass set (`-O1`/targeted) or a bigger asyncify removelist cuts that floor; needed
   to get toward ~30 min. Validate output in Chrome (it exists for V8's locals limit).

Everything below this section about "madvise TLB-shootdowns" and "THP compaction"
was the pre-measurement hypothesis and is **WRONG for this workload** — kept only as
the reasoning trail. Trust this section.

## (superseded hypothesis) The memory-storm theory
The `-O2` pass is **not** CPU-bound on optimization work. It is bound by a
**kernel page-management storm** on glibc Linux: the allocator constantly returns
freed pages to the OS (`madvise(MADV_DONTNEED)`/`munmap`), and each return forces
**cross-core TLB-shootdown work** (plus, on Ubuntu 24.04, very likely
Transparent-Huge-Page compaction). That work is **system (kernel) time**, it
**scales super-linearly with thread count**, and it is **largely allocator-choice
independent** — which is exactly why swapping in *default* jemalloc only helped
~15%. [SUPERSEDED: on-box perf shows the system time is futex spinlock, not
TLB-shootdowns; madvise=0, compaction=0. The "futex" the prior strace saw was
lock contention, not allocator arenas. See the VERDICT above.]

## The measurements that settle it
Per-pass `/usr/bin/time -v`, pulled from the real CI runs
(`emergence-engineering/pcbjam`, workflow "CI"). The earlier table omitted the
**context-switch and page-fault counters — those are the diagnostic gold.**

### `wasm-opt -O2` pass (the ~80-min bottleneck)
| metric | 8c glibc (run 27139529490) | 32c glibc (27144910231) | 32c jemalloc-default (27186569662) |
|---|---|---|---|
| BINARYEN_CORES | 8 | 32 | 32 |
| wall clock | **1:23:57** (5037 s) | **1:28:30** (5310 s) | 1:16:23 (4583 s) |
| user time | 12,807 s | 13,043 s | 12,177 s |
| **system time** | **18,030 s (58%)** | **114,075 s (90%)** | **95,525 s (89%)** |
| % CPU | 612% | 2393% | 2349% |
| peak RSS | 39.8 GB | 39.7 GB | 39.0 GB |
| minor page faults | 73.7 M | 67.2 M | 69.7 M |
| **voluntary ctx-switches** | **674,829,131** | **180,672,277** | **149,367,474** |
| involuntary ctx-switches | 31,675 | 1,235,328 | 1,429,770 |

### `--asyncify` pass (same machine, only ~6 GB RSS — shows the storm too)
| metric | 8c glibc | 32c glibc | 32c jemalloc-default |
|---|---|---|---|
| wall | 9:26 (566 s) | **10:51 (651 s)** | 8:28 (508 s) |
| user / system | 1,317 / 2,715 s | 1,187 / **19,045 s** | 994 / 14,630 s |
| voluntary ctx-switches | 61.8 M | 10.4 M | 7.3 M |

### What these numbers prove
1. **Cores anti-scale.** Under the *same* allocator, 32c is *slower* than 8c
   (-O2: 5310 vs 5037 s; asyncify: 651 vs 566 s). System time scales **~6.3×
   for a 4× core bump** (18k→114k) while user time stays flat. That super-linear-
   in-cores, flat-in-user-work shape is the fingerprint of **cross-core kernel
   coordination (TLB shootdowns / compaction)**, not of the optimization work.
2. **Only ~2.5 cores of real work ever happen.** user ÷ wall ≈ 12,800 ÷ 5,000 ≈
   **2.5** at both 8c and 32c. `wasm-opt -O2` barely parallelizes on this module
   (a few asyncify-created monster functions dominate — Amdahl). The other ~20
   "busy" cores at 32c are burning **kernel** time, not optimizing.
3. **The contention is allocator-independent.** glibc and jemalloc both sit at
   **150–680 M voluntary context-switches** and ~70 M page faults; jemalloc-
   default shaved only ~16% of system time and ~14% of wall. A real arena-lock
   problem would have collapsed under jemalloc. It didn't → the cost is **not**
   in the allocator's arenas.
4. **The ctx-switch *inversion* (8c=675 M vs 32c=181 M, yet 32c has 6× the system
   time)** means at 32c threads stop *sleeping* on locks and instead *spin in the
   kernel* (TLB-shootdown IPIs / page-table locks) — consistent with the
   shootdown model, not userspace mutex spinning (which would be *user* time).

## Root cause
**A kernel virtual-memory storm driven by allocator page-return traffic.** Both
glibc and (default) jemalloc periodically hand freed pages back to the OS via
`madvise(MADV_DONTNEED)`/`munmap`. On x86-64 each such return triggers a
**TLB shootdown** — the OS sends IPIs to the other cores running the process's
threads to flush their TLBs — which costs more the more cores exist (hence the
6.3× system-time blow-up 8c→32c). On Ubuntu 24.04 with THP active, faulting
threads can additionally stall in **direct compaction** (`__alloc_pages_slowpath`
→ `try_to_compact_pages`) plus background `khugepaged`. macOS doesn't hit this
(different VM/TLB + allocator, bare-metal, only 10 cores), which is why the
weaker Mac is faster. **Which term dominates — shootdowns vs compaction — was not
yet measured on this exact module; Phase 0 of the experiment plan measures it.**

## Deep-research corroboration (primary sources)
A fan-out research pass (Linux kernel THP docs, jemalloc/mimalloc tuning docs,
glibc-maintainer write-up, Binaryen issues) independently reached the same
diagnosis and supplied the key precedent:
- **Binaryen #5561** — 48-core AMD EPYC, 128 GB, Ubuntu: `wasm-opt` went
  **58m35s → 3m43s** (system time 2,395 min → 40 s) just by switching the
  allocator to **mimalloc** (which by default purges far less aggressively).
  Same anti-scaling signature as ours. This is the upside ceiling.
- **Binaryen #6338** — a 10× `wasm-opt` slowdown "fixed by using Emscripten's
  mimalloc port." Multiplicative, not marginal.
- jemalloc `TUNING.md`: decay time is "a trade-off between CPU and memory" — the
  default *keeps* issuing `madvise`, which is precisely why a default jemalloc
  swap didn't help. The fix is to **disable decay** (retain memory).
- Live alternative hypothesis (do not ignore): Binaryen's own `wasm::Type` global
  mutex (mutrace: 41.8 M locks / 10.7 M contentions in #5561) can contribute
  futex/system time **independent of malloc** — unfixed by any allocator/THP
  change. If allocator-retain + THP-off underperform, this is the next suspect,
  and it points at **upgrading Binaryen** (see below).

### Corrections from adversarial verification (don't repeat these overclaims)
- ❌ "MADV_DONTNEED broadcasts IPIs to *all* CPUs and is KVM-amplified" — refuted.
  Shootdowns go only to cores that ran the process's threads; no special VM
  penalty was substantiated. (Still scales with thread count.)
- ❌ "`dirty_decay_ms:-1` is the documented official fix and fully stops madvise"
  — failed verification; one report saw madvise persist anyway. **Every retain
  config must be strace/perf-verified to confirm madvise actually drops to ~0.**
- ❌ "THP high-system-time symptom definitely matches ours" — the *mechanism* is
  well-documented but attribution to THP for *this* module is an inference;
  measure it (Phase 0), don't assume.

## Levers, ranked (exact flags)
All are env/sysctl only (no pipeline change) except D/E. 128 GB RAM vs ~40 GB
peak makes "never return memory" safe.

| # | Lever | Exact change | Confidence | Risk |
|---|---|---|---|---|
| **A** | Fewer threads | `BINARYEN_CORES=8` (sweep 4–16) | High — *already in our data* (8c ties/beats 32c) | none |
| **B** | Allocator retain (KILLS the purge) | jemalloc `MALLOC_CONF=dirty_decay_ms:-1,muzzy_decay_ms:-1,background_thread:true` · mimalloc `MIMALLOC_PURGE_DELAY=-1` · glibc `MALLOC_TRIM_THRESHOLD_=-1 MALLOC_MMAP_MAX_=0` | High mechanism; **must verify madvise→0** | low (more RAM) |
| **C** | Disable THP | `echo never | sudo tee /sys/kernel/mm/transparent_hugepage/{enabled,defrag}` | High mechanism | low |
| **B+C** | Stack | retain allocator + THP=never | High | low |
| **D** | Newer Binaryen / bundled mimalloc | bump `BINARYEN_VERSION` past 121 (`scripts/common/get-wasm-opt.sh`) | Med (#5561/#6338) | **Med-High**: must match the emsdk Binaryen or asyncify metadata corrupts ("func is not a function"); needs e2e |
| **E** | Shrink the `-O2` input | bigger `asyncify-removelist` / `-O1`/targeted passes vs full `-O2` | Med | Med — needs Chrome e2e per change |
| — | Contention-only controls (expected to NOT fix it) | `MALLOC_ARENA_MAX=4`, `MIMALLOC_PURGE_DECOMMITS=0` | — | — |

**Lever / merely-reduces-contention distinction:** retain configs (B) *kill* the
madvise/munmap traffic; `MALLOC_ARENA_MAX` only trims arena-lock cost (which
isn't our bottleneck) — included as a control to confirm the diagnosis.

## Expected outcome / the 30-min question
At 8c, if `-O2` parallelized perfectly with zero system time it'd be ~27 min
(12,807 s ÷ 8 ÷ 60). The gap to 84 min is the storm **plus** parallelism capped
at ~2.5× — and 32c tying 8c implies **contention is what caps the parallelism**.
So killing the storm should cut system time *and* unlock real core scaling →
**~30 min is plausible** but bounded by Binaryen's true serial fraction (unknown
until measured). Going clearly below likely also needs Lever D and/or E. The
#5561 15.8× is a ceiling demonstration, not a prediction (our *user* time is real).

## The experiment plan (single Hetzner slot at a time)
Running one full CI per config is wasteful (re-compile + e2e). Instead a
dedicated bench builds the `-O2` **input once**, caches it as an artifact, and
replays the sweep over it — many data points per slot, later runs skip the build.

- **Phase 0 (in run #1):** run `baseline` with `DIAGNOSTIC=1` to attribute the
  kernel time. Decision rule:
  - `native_flush_tlb_multi` / `smp_call_function_many` high + `madvise` flood →
    **Lever B** (allocator retain).
  - `try_to_compact_pages` / `compaction_*` / `__alloc_pages_slowpath` + rising
    `/proc/vmstat:compact_stall` → **Lever C** (THP=never).
  - `wasm::Type::*` / futex with **no** madvise flood → **Lever D** (Binaryen ver).
- **Phase 1:** sweep B / C / B+C over the cached fixture; confirm each drops
  voluntary ctx-switches and `madvise` count by 10–100×.
- **Phase 2:** validate the winner end-to-end (full build + Chrome e2e) — `-O2`
  exists to keep asyncify functions under V8's locals limit, so a faster config
  that corrupts the module is worthless.

## The bench harness (how the next agent runs it)
Added in this branch:
- **`.github/workflows/wasm-opt-bench.yml`** — ephemeral Hetzner ccx53, builds-or-
  downloads the fixture, runs the sweep, uploads `o2-bench-results-<run_id>`.
  Triggers on **push to `bench/**`** (can't collide with main CI `[main]` or the
  feature-branch Hetzner CI). `concurrency: wasm-opt-bench` (one VM at a time).
- **`scripts/bench/o2-config-sweep.sh`** — replays `wasm-opt -O2` over the fixture
  under each preset; records `time -v` counters + a perf-stat `madvise/munmap`
  count; `DIAGNOSTIC=1` adds vmstat/interrupts deltas + a perf kernel-symbol
  sample. Preset menu is in the script's `config_env`.
- **`scripts/bench/sweep.conf`** — committed run parameters (CONFIGS / CORES /
  FIXTURE_RUN_ID / DIAGNOSTIC). Edit + commit + push to `bench/**` to launch.

**Drive it:**
1. Run #1 (this branch): `CONFIGS_CONF="baseline mimalloc-retain"`,
   `FIXTURE_RUN_ID_CONF=""` (builds + caches fixture), `DIAGNOSTIC_CONF=1`.
2. Note run #1's id → set `FIXTURE_RUN_ID_CONF` to it so later runs skip the
   ~40-min build, then sweep `"thp-off jemalloc-retain glibc-retain
   thp-off+mimalloc-retain"`.
3. Core sweep the winner: `CONFIGS="<winner>"`, `CORES_CONF=8` (then 16, 4).

Pull results: `gh run download <run_id> -n o2-bench-results-<run_id>`, read
`results.csv` (the `vol_ctxsw`, `sys_s`, and `madvise` columns tell the story).

## Why local QEMU is NOT a fair proxy (asked & answered)
The existing `scripts/bench/` QEMU harness can only run the *asyncify* pass
locally — three independent blockers for `-O2`: (1) **RAM** — `-O2` needs ~40 GB,
a 32 GB Mac caps a guest at ~20 GB → OOM; (2) **arch** — the storm is x86 IPI
TLB-shootdowns, but Apple Silicon is aarch64 with hardware-broadcast `TLBI`
(different mechanism/scaling); (3) **core count** — can't reach the 32-core regime
where it's worst. Use QEMU only for the free "does this config stop madvise"
smoke test on the asyncify pass; do the real numbers on the disposable ccx53.

## Branch / repo state at handoff
- Bench work on branch **`bench/wasm-opt-allocator-sweep`** (off
  `istvanmatejcsok/feat/ci-hetzner-allcores`). Adds the three files above.
- CI uses **standalone Binaryen v121** (`get-wasm-opt.sh` fallback; CI has no
  local emsdk). Lever D must keep this in sync with the emsdk Binaryen.
- Repo `emergence-engineering/pcbjam`. Main CI runs on **Ubicloud** (`[main]`);
  the Hetzner ccx53 only spins for the two `ci-hetzner` feature branches and the
  new `bench/**` branch. "One Hetzner slot at a time" is the operative limit.
- `apply-asyncify.sh`: jemalloc auto-preload + `ASYNCIFY_ONLY=1` (stops before
  -O2 — used to build the bench fixture) + `WASM_OPT_PRELOAD=none` sentinel.

## Useful commands
- Pull per-pass timing from a finished CI run's live log:
  `gh run view <id> --repo emergence-engineering/pcbjam --log | grep -iE "Running wasm-opt|User time|System time|Elapsed \(wall|Maximum resident|context switches|page faults"`
- Older runs (no live log): `gh run download <id> -n e2e-logs` → `logs/build/*.log`.
- Launch a bench run: edit `scripts/bench/sweep.conf`, commit, `git push` to the
  `bench/**` branch.
