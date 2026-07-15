# AROS Nightly Build — Staged Build Implementation Guide

> **Implemented:** the design below is realised in this directory
> (`scripts/azure-new/`). See `README.md` for the operational quick reference;
> this document is the full rationale and analysis.

**Status:** Design / implementation guide
**Scope:** `scripts/azure/azure-pipelines.yml` and the AROS build system it drives
**Goal:** Keep local *all‑in‑one* builds working unchanged, while reworking the
Azure nightly pipeline into *staged* builds so that no single agent has to hold
the toolchain build **and** the core build **and** contrib on one disk at once.

---

## 0. Executive summary

The nightly pipeline runs the **entire** build (cross‑toolchain → core system →
distfiles → contrib → package → deploy) as **one Azure job on one agent**. Every
intermediate tree therefore coexists on a single, storage‑constrained disk. The
two observed failures are a direct consequence:

* **LLVM/Clang toolchain** runs out of space while the huge LLVM build tree is
  being compiled — and it is rebuilt **from scratch on every nightly**.
* **GNU contrib** runs out of space because contrib is compiled *in place on top
  of* the already‑built core, *plus* a full duplicate copy of the core tree
  (`AROS.precontrib`), *plus* the freshly‑built base/boot distfiles, *plus* all
  extracted port sources and contrib object files — simultaneously.

The fix does **not** require changing how a developer builds locally. The AROS
build system already exposes every primitive needed to stage a build
(prebuilt‑toolchain install/reuse, shared ports sources, independent
`crosstools` / core / `distfiles` / `contrib` make targets, and per‑category
`contrib-*` sub‑targets). The rework is therefore almost entirely in the **Azure
YAML**: split the single job into **dependent stages**, each on a fresh agent,
handing the toolchain (and only the toolchain) forward as a cached artifact.

Local `configure && make` is untouched.

---

## 1. Review of the current nightly build system

### 1.1 Moving parts

| Area | Files | Role |
|------|-------|------|
| Main pipeline | `scripts/azure/azure-pipelines.yml` | Builds **one** flavour, parameterised by `arosbuild.*` pipeline variables. |
| Aux pipelines | `scripts/azure/update-nightlies.yml`, `gen-unittests.yml`, `update-dev-nightlies.yml`, `gen-downloads.py`, `check-translations.yml`, `rss-commits.yml` | Post‑processing on SourceForge (assemble nightly dir, regen download/unittest pages). Not the bottleneck. |
| Legacy nightly | `scripts/nightly/` (`build`, `setup`, `cfg/*`, `pkg/*`) | The **previous** nightly system — already a *staged, per‑package* model. Excellent prior art (see §1.4). |
| Build system | `Makefile.in`, `configure.in`, `tools/crosstools/**`, `arch/**`, `config/**`, contrib `mmakefile.src` | The actual build. Already supports staging primitives. |

The per‑flavour pipeline definitions (the `arosbuild.*` variable values, the
`vmImage`, etc.) live **in the Azure DevOps UI**, not in the repo — each flavour
is a separate pipeline that points at the same `azure-pipelines.yml`. Azure
automatically exposes each `arosbuild.x.y` variable to scripts as the env var
`AROSBUILD_X_Y`, which the YAML relies on (e.g. `$AROSBUILD_TARGET`,
`$AROSBUILD_TOOLCHAIN_FAMILY`).

### 1.2 The current pipeline flow (single job `Build`)

All of the following run **sequentially on one agent** (`azure-pipelines.yml`):

1. SSH key setup; choose toolchain family (`gnu`/`llvm`).
2. Install host build dependencies (apt/brew); assemble `AROSCONFIGOPTIONS`.
3. Detect `BUILDTHREADS` (`nproc`).
4. *(Darwin only)* build native host GCC.
5. Setup workspace; `git clone` contrib; create the common dirs under
   `AZBUILDPATH = $(Build.BinariesDirectory)`:
   * `toolchain/` (`AROSBUILDTOOLCHAINDIR`) — toolchain install dir
   * `builds/` (`AROSBUILDSDIR`) → `builds/<name>` (`AROSBUILDDIR`) — build dir
   * `portssources/`, `binaires/`, `logs/`, `distfiles/`
6. Tar + upload **source** and **contrib‑source** packages.
7. **Configure toolchain:** `configure … --with-aros-toolchain-install=$(AROSBUILDTOOLCHAINDIR)`.
8. **Build toolchain:** `make crosstools` → tar `toolchain/` → `rm -Rf $(AROSBUILDDIR)/*`.
   *(Unconditional — runs every nightly, LLVM included.)*
9. Publish + upload toolchain.
10. **Re‑configure for AROS:** `configure … --with-aros-toolchain-install=… --with-aros-toolchain=yes`.
11. **Build core:** `make`.
12. **Boot distfiles:** `make boot-distfiles` → `distfiles-boot/`.
13. **Main distfiles:** `make distfiles` → `distfiles/`.
14. **Contrib:** `cp -r …/AROS …/AROS.precontrib`; move contrib into `$(AROSSRCDIR)/contrib`; `make contrib`.
15. **Package:** zip/lha/tar the boot, base and contrib trees (contrib via a
    recursive diff of `AROS` vs `AROS.precontrib`).
16. Logs, publish artifacts, rsync everything to SourceForge.
17. *(optional)* Hosted CUnit unit tests.

### 1.3 Build‑system staging primitives that **already exist**

These are the levers the rework uses. None of them are new.

* **Build a toolchain into a chosen dir:**
  `./configure --with-aros-toolchain-install=DIR … && make crosstools`
  installs the cross compiler into `DIR` (`AROS_CROSSTOOLSDIR`, `configure.in:2500`).
* **Reuse a prebuilt toolchain (skip building it):**
  `./configure --with-aros-toolchain=yes --with-aros-toolchain-install=DIR …`
  (`configure.in:2381`). With `=yes`, `crosstools` becomes a no‑op as long as the
  binaries exist in `DIR`.
* **Share downloaded port sources:** `--with-portssources=DIR` (`configure.in:2923`).
* **Independent top targets:** `crosstools`, `sdk`, (`make` = all), `boot-distfiles`,
  `distfiles`, `contrib` (`Makefile.in:125,180,330,346,360`).
* **Contrib decomposes into categories** — each depends only on `includes linklibs`
  (the SDK), *not* the full system (`config/mmakefile.src:19`,
  contrib `mmakefile.src`):
  `contrib-aminet`, `contrib-fish`, `contrib-games`, `contrib-misc`,
  `contrib-demo`, `contrib-dopus`, `contrib-bgui`, `contrib-gfx`,
  `contrib-development`, `contrib-gnu`, `contrib-networking`, `contrib-pack`,
  `contrib-scout`, `contrib-sdl2`, `contrib-multimedia`, `contrib-workbench`,
  `contrib-fryingpan-$(cpu)`. Per‑target `*-quick` and `*-clean` variants exist.
* **Toolchain install is idempotent:** `.installflag-*` stamps under
  `CROSSTOOLSDIR` (`tools/crosstools/llvm/mmakefile.src:80‑85`, `Makefile.in:175`)
  mean a *restored* toolchain dir is not rebuilt.
* **LLVM already builds component‑by‑component** (`LLVM` → `lld` → `clang` →
  tools), running `make clean` between each to bound disk
  (`tools/crosstools/llvm/mmakefile.src:503‑539`).

### 1.4 Prior art — the legacy `scripts/nightly` per‑package model

The previous nightly system **already staged** the build and is the template for
this rework:

* `cfg/<server>` lists `CFG_PACKAGES=(sdk.svn.pc-x86_64 contrib.svn.pc-x86_64 …)`
  and `CFG_PREBUILD_TOOLCHAIN_PACKAGE=crosstools.svn.pc-x86_64`.
* `scripts/nightly/build` builds the toolchain package **first**, installs it to a
  **shared** `TOOLCHAIN_BASE`, then builds each remaining package
  **in its own fresh copy of the source**, deleting it afterwards.
* Each package (`pkg/sdk`, `pkg/contrib`, `pkg/hosted`, …) configures with
  `--with-aros-toolchain-install` + `--with-aros-toolchain=yes` +
  `--with-portssources`, builds only what it needs, packages, then
  `delete $BUILD_BASE/AROS`.
* Crucially, **`pkg/contrib` does not need the core build tree** — it runs
  `make tools mmake contrib` in a clean tree; MetaMake pulls in just
  `includes`+`linklibs`. The contrib stage is *self‑contained*.

That single property — *contrib can be built against a prebuilt toolchain
without the core build tree* — is what makes disk‑bounded staging possible.

---

## 2. Root‑cause analysis of the disk failures

### 2.1 One agent, one disk, cumulative peak

Microsoft‑hosted agents are storage‑constrained (Microsoft documents only a
modest guaranteed free allowance, and the work tree — `Build.BinariesDirectory`,
where `AZBUILDPATH` lives — sits on the OS disk). Because the whole build is one
job, the disk must simultaneously hold, at the contrib step:

```
toolchain/                      (kept for the whole job)
builds/<name>/bin/<t>/AROS      (built core)
builds/<name>/bin/<t>/AROS.precontrib   (a *full copy* of the above)
builds/<name>/bin/<t>/gen       (all core .o)
builds/<name>/distfiles-boot/   + base distfiles/  (already assembled)
AROS-<date>-<name>-*.{lha,iso…} (already‑built base + boot archives)
portssources/ + gen/contrib/    (extracted port sources + contrib .o)
ccache
```

The peak is the **sum** of every phase. That is the core problem.

### 2.2 Toolchain (LLVM) — rebuilt every night, build tree is huge

The `make crosstools` step (§1.2 step 8) has **no `condition:`** — LLVM is
rebuilt from source on every run. An LLVM build tree is tens of GB even with the
per‑component `make clean`. On a shared disk that also carries the source/contrib
packages, it tips over. The toolchain, however, changes only when
`tools/crosstools/**` or the requested gcc/llvm version changes — i.e. rarely.

### 2.3 Contrib — built on top of core, doubled, alongside distfiles

Step 14 copies the entire built `AROS` tree to `AROS.precontrib` purely to later
*diff* which files contrib added. That **doubles** the core footprint at exactly
the moment contrib’s own sources and objects are at their largest, and it happens
while the base + boot distfiles and their archives are still on disk. `contrib-gnu`
(GCC/binutils ports etc.) is among the heaviest categories and is where it tends
to fail.

### 2.4 Output lands on the small OS disk

`AZBUILDPATH = $(Build.BinariesDirectory)` → `/home/vsts/work/1/b`, on the OS
disk. The pipeline already prints `df -H` (`azure-pipelines.yml:84`) — read it in
a recent run to see the real numbers for your image, and relocate if a larger
volume exists (§5.5).

---

## 3. Design goals

1. **Do not change local builds.** `./configure … && make && make contrib`
   keeps working exactly as today. Staging is an Azure‑orchestration concern only.
2. **Bound per‑agent peak disk.** No agent holds more than one heavy tree at a
   time.
3. **Stop rebuilding the toolchain every night.** Build it only when its inputs
   change; otherwise restore it from cache/artifact.
4. **Independent, restartable stages** with an explicit artifact hand‑off
   contract, so a failed contrib stage doesn’t cost you the toolchain/core.
5. **Reuse existing primitives** (§1.3) rather than inventing build‑system
   features.

---

## 4. Proposed architecture — staged Azure pipeline

### 4.1 Stage graph

```
           ┌─────────────────────┐
           │  Stage A: Toolchain │   fresh agent; build crosstools ONCE,
           │  (cached across runs)│  publish toolchain artifact + cache
           └──────────┬──────────┘
                      │ toolchain artifact
        ┌─────────────┴──────────────┐
        ▼                            ▼
┌───────────────────┐      ┌─────────────────────┐
│ Stage B: Core      │      │ Stage C: Contrib    │   each: fresh agent,
│ (boot+main distfiles)│    │ (self-contained;    │   download toolchain,
│ → base/boot pkgs   │      │  optional per-cat.) │   reuse it, build, package
└─────────┬─────────┘      └──────────┬──────────┘
          │ distfiles artifacts        │ contrib artifact
          └──────────────┬─────────────┘
                         ▼
              ┌──────────────────────┐
              │ Stage D: Deploy +     │  download all artifacts,
              │ Unit tests            │  rsync to SourceForge, CUnit
              └──────────────────────┘
```

Stages B and C both depend only on A, so Azure can run them **in parallel** on
two agents (each with a full, fresh disk) — or serially if you prefer fewer
concurrent agents. Either way the **peak per agent** drops to “one heavy tree”.

### 4.2 Stage A — Toolchain (build once, cache across runs)

* Fresh agent. Configure with `--with-aros-toolchain-install=$(AROSBUILDTOOLCHAINDIR)`,
  `make crosstools`, tar `toolchain/`.
* Wrap it in **pipeline caching** keyed on the toolchain’s real inputs so the
  LLVM/GCC build runs only on a cache miss:

  ```
  key = aros-toolchain | $(arosbuild.name) | <family> | <gcc/llvm version>
            | hashFiles('tools/crosstools/**')
  path = $(AROSBUILDTOOLCHAINDIR)
  ```

* On a cache **hit**, `make crosstools` is a no‑op (the `.installflag-*` stamps
  are restored with the dir) — no LLVM compile, no disk blow‑up.
* On a cache **miss**, the LLVM tree is the *only* large consumer on the agent
  (no contrib, no distfiles competing), and the existing per‑component
  `make clean` keeps it within a fresh agent’s disk.
* **Relocatability caveat:** `configure` bakes the absolute `CROSSTOOLSDIR` into
  generated config/specs/wrappers (`AC_SUBST(AROS_CROSSTOOLSDIR)`), and GCC bakes
  its `--prefix`. **Always restore the toolchain to the same absolute path** it
  was built at (`$(Build.BinariesDirectory)/toolchain`, which is stable on a
  given image), and **re‑run `configure --with-aros-toolchain-install=<that path>`**
  in stages B/C (which regenerates the wrappers without rebuilding the compiler).

Publishes: `toolchain-$(arosbuild.name)` artifact (also feeds the existing
SourceForge `Developer_Tools` upload, gated by `arosbuild.withtoolchain`).

### 4.3 Stage B — Core + distfiles

* Fresh agent. **Download** the toolchain artifact to
  `$(AROSBUILDTOOLCHAINDIR)`.
* `configure … --with-aros-toolchain-install=$(AROSBUILDTOOLCHAINDIR) --with-aros-toolchain=yes`.
* `make` → `make boot-distfiles` → `make distfiles`.
* Package base + boot exactly as today; publish `distfiles-$(arosbuild.name)`.
* Contrib is **not** built here, so the contrib sources/objects never touch this
  agent.

### 4.4 Stage C — Contrib (self‑contained)

This is **Option A** (recommended), mirroring legacy `pkg/contrib`:

* Fresh agent. Download the toolchain artifact (same absolute path).
* Move contrib sources into `$(AROSSRCDIR)/contrib`.
* `configure … --with-aros-toolchain=yes --with-aros-toolchain-install=… --with-portssources=…`.
* Build contrib. MetaMake builds only `includes`+`linklibs` first, then contrib —
  the full core ROM/distfiles are **never** built or present here.
* **Replace the full `AROS.precontrib` copy with a file manifest** (§5.4). For an
  *exact* match to today’s packaging, have Stage B publish a small text
  **base‑file manifest** (a `find`‑generated list of the fully‑built base tree),
  and in Stage C compute `contrib = (files present after contrib build) − (base
  manifest)`. This is byte‑equivalent to the current “diff against a full
  `AROS.precontrib`” logic but costs a few KB instead of a full second copy of
  the SDK tree. (A purely local snapshot taken *after* `includes`+`linklibs` is a
  simpler fallback, but can slightly over‑include components that a full core
  would have placed in the base — see §5.4.)
* Package + publish `contrib-$(arosbuild.name)`.

Peak disk here = toolchain + includes/linklibs + extracted port sources +
contrib objects. No base distfiles, no boot ISO, no duplicate core tree.

**Optional finer staging (for the heaviest categories / smallest agents):**
because each `contrib-<category>` depends only on `includes linklibs`, you can
split Stage C into per‑category steps that *package then clean before the next*:

```bash
for cat in aminet fish games misc demo dopus bgui gfx development \
           gnu networking pack scout sdl2 multimedia workbench; do
  make -j$(BUILDTHREADS) contrib-$cat
  # …collect this category's new files into the contrib staging dir…
  make contrib-$cat-clean 2>/dev/null || true
  rm -rf "$(AROSPORTSSRCSDIR)"/*/build* gen/contrib/$cat   # drop extracted srcs/objs
done
```

This keeps only one category’s footprint live at a time and directly addresses
the `contrib-gnu` out‑of‑space failure.

### 4.5 Stage D — Deploy + unit tests

* Download `distfiles-*` and `contrib-*` (and source) artifacts.
* rsync to SourceForge `uploads/nightly2/` (unchanged logic).
* Run the hosted CUnit phase (unchanged), gated by `arosbuild.withunittests`.

### 4.6 Artifact hand‑off contract

| Producer | Artifact | Path restored in consumer | Consumer |
|----------|----------|---------------------------|----------|
| A | `toolchain-<name>` (`toolchain/`) | `$(AROSBUILDTOOLCHAINDIR)` (same abs path) | B, C |
| B | `distfiles-<name>` (base/boot pkgs + md5) | staging | D |
| C | `contrib-<name>` (contrib pkg + md5) | staging | D |
| (A/B/C) | `sources-<name>` (already produced once) | staging | D |
| (A/B/C) | `logs-<name>` | staging | D / always() |

Cross‑stage hand‑off uses **Pipeline Artifacts** (`publish:` / `download:` or
`PublishPipelineArtifact@1` / `DownloadPipelineArtifact@2`) — the current
`PublishBuildArtifacts@1` + `Build.ArtifactStagingDirectory` pattern is
per‑job and must be switched to pipeline artifacts (or `download:` auto‑fetch)
to cross stage boundaries.

### 4.7 Why local all‑in‑one is unaffected

Nothing above changes `configure`, `Makefile.in`, or any `mmakefile.src`. A
developer still runs the same `./configure … && make && make contrib`. The
stages are just the existing top‑level targets invoked on separate agents with a
prebuilt toolchain — precisely what a normal build does internally.

### 4.8 Alternatives considered

* **Option B — pass the core build tree to contrib.** Stage B publishes the
  configured `bin/<target>` tree; Stage C downloads it and runs `make contrib`
  on top (no core recompile). **Rejected as default:** it ships a multi‑GB
  artifact and keeps Stage C’s peak ≈ current (core tree + contrib). Use only if
  contrib recompile time becomes the binding constraint and disk is ample.
* **Dependent jobs instead of stages.** If you want to keep a single logical
  pipeline run with simpler fan‑out, use `jobs:` with `dependsOn` +
  pipeline artifacts within one stage. Functionally similar; stages give cleaner
  gating, retries and UI grouping. Either satisfies the disk requirement.
* **Single job, just relocate to a bigger disk + cache toolchain + drop the
  precontrib copy.** The cheapest partial mitigation (§5.4–5.5). Helps, but does
  **not** remove the cumulative‑peak problem (§2.1). Recommended *in addition to*
  staging, not instead of it.

---

## 5. Concrete implementation

> The snippets below are templates to adapt into `scripts/azure/`. They reuse
> the existing variable names so the per‑flavour pipeline definitions barely
> change.

### 5.1 New / changed variables

| Variable | Meaning | Default |
|----------|---------|---------|
| `arosbuild.stage` | (optional) force a single stage when reusing the YAML as a job template: `toolchain` \| `core` \| `contrib` \| `deploy` | unset = full graph |
| `AZBUILDPATH` | workspace root | `$(Build.BinariesDirectory)` (override to a larger volume — §5.5) |
| `TOOLCHAIN_CACHE_KEY` | computed cache key for Stage A | see §4.2 |

The existing `arosbuild.withtoolchain` / `withcontrib` / `withunittests` /
`deploy` gates are reused to enable/skip stages.

### 5.2 Refactor `azure-pipelines.yml` into stage/step templates

Extract the repeated logic (the `execute()` logging wrapper, the SSH setup, the
dep install, the packaging) into **step templates** so each stage is small:

```
scripts/azure/
  azure-pipelines.yml            # now a stages: orchestrator
  templates/
    steps-common-env.yml         # SSH key, toolchain family, df -H
    steps-install-deps.yml       # apt/brew + AROSCONFIGOPTIONS assembly
    steps-workspace.yml          # mkdir tree, clone contrib, BUILDDATE
    steps-toolchain.yml          # configure+make crosstools (+cache)
    steps-core.yml               # configure+make+boot-distfiles+distfiles+pkg
    steps-contrib.yml            # configure+make contrib(+per-cat)+pkg
    steps-deploy.yml             # rsync + unit tests
```

Orchestrator skeleton:

```yaml
trigger: [ master ]

stages:
- stage: Toolchain
  jobs:
  - job: build_toolchain
    timeoutInMinutes: 360
    pool: { vmImage: '$(arosbuild.vmimage)' }
    variables:
      AZBUILDPATH: '$(Build.BinariesDirectory)'
      AROSBUILDTOOLCHAINDIR: '$(AZBUILDPATH)/toolchain'
    steps:
    - template: templates/steps-common-env.yml
    - template: templates/steps-install-deps.yml
    - template: templates/steps-workspace.yml
    - task: Cache@2
      inputs:
        key: 'aros-toolchain | "$(arosbuild.name)" | "$(TOOLCHAIN)" | "$(arosbuild.gcc.version)$(arosbuild.llvm.version)" | tools/crosstools/**'
        path: '$(AROSBUILDTOOLCHAINDIR)'
        cacheHitVar: TOOLCHAIN_RESTORED
    - template: templates/steps-toolchain.yml   # internally: if not TOOLCHAIN_RESTORED, configure+make crosstools
    - publish: '$(AROSBUILDTOOLCHAINDIR)'
      artifact: 'toolchain-$(arosbuild.name)'

- stage: Core
  dependsOn: Toolchain
  jobs:
  - job: build_core
    timeoutInMinutes: 360
    pool: { vmImage: '$(arosbuild.vmimage)' }
    variables:
      AZBUILDPATH: '$(Build.BinariesDirectory)'
      AROSBUILDTOOLCHAINDIR: '$(AZBUILDPATH)/toolchain'
    steps:
    - template: templates/steps-common-env.yml
    - template: templates/steps-install-deps.yml
    - template: templates/steps-workspace.yml
    - download: current
      artifact: 'toolchain-$(arosbuild.name)'
    - script: |   # restore to the SAME absolute path the toolchain was built at
        mkdir -p '$(AROSBUILDTOOLCHAINDIR)'
        cp -a '$(Pipeline.Workspace)/toolchain-$(arosbuild.name)/.' '$(AROSBUILDTOOLCHAINDIR)/'
      displayName: 'Restore prebuilt toolchain'
    - template: templates/steps-core.yml
    - publish: '$(AROSBUILDBINDIR)'
      artifact: 'distfiles-$(arosbuild.name)'

- stage: Contrib
  dependsOn: Toolchain
  condition: and(succeeded(), eq(variables['arosbuild.withcontrib'], 'yes'))
  jobs:
  - job: build_contrib
    timeoutInMinutes: 360
    pool: { vmImage: '$(arosbuild.vmimage)' }
    variables:
      AZBUILDPATH: '$(Build.BinariesDirectory)'
      AROSBUILDTOOLCHAINDIR: '$(AZBUILDPATH)/toolchain'
    steps:
    - template: templates/steps-common-env.yml
    - template: templates/steps-install-deps.yml
    - template: templates/steps-workspace.yml
    - download: current
      artifact: 'toolchain-$(arosbuild.name)'
    - script: |
        mkdir -p '$(AROSBUILDTOOLCHAINDIR)'
        cp -a '$(Pipeline.Workspace)/toolchain-$(arosbuild.name)/.' '$(AROSBUILDTOOLCHAINDIR)/'
      displayName: 'Restore prebuilt toolchain'
    - template: templates/steps-contrib.yml
    - publish: '$(AROSBUILDBINDIR)'
      artifact: 'contrib-$(arosbuild.name)'

- stage: Deploy
  dependsOn: [ Core, Contrib ]
  condition: and(succeededOrFailed(), ne(variables['arosbuild.deploy'], 'no'))
  jobs:
  - job: deploy
    pool: { vmImage: '$(arosbuild.vmimage)' }
    steps:
    - download: current      # pulls distfiles-* and contrib-*
    - template: templates/steps-deploy.yml
```

`steps-toolchain.yml` guards the heavy work with the cache var so a hit skips the
LLVM/GCC build entirely:

```yaml
steps:
- script: |
    if [ "$TOOLCHAIN_RESTORED" = "true" ]; then
      echo "Toolchain restored from cache — skipping crosstools build."; exit 0
    fi
    # …existing execute() wrapper…
    execute $(AROSSRCDIR)/configure $(AROSCONFIGOPTIONS) --with-aros-toolchain-install=$(AROSBUILDTOOLCHAINDIR)
    execute make -j $(BUILDTHREADS) crosstools
    rm -Rf $(AROSBUILDDIR)/*           # keep only toolchain/
  workingDirectory: '$(AROSBUILDDIR)'
  displayName: 'Build $(arosbuild.name) $(TOOLCHAIN) toolchain (cache miss)'
```

### 5.3 Toolchain caching details

* Add `displayName`d `Cache@2` (restore) before, and rely on its post‑job save.
* Key must include the **family** and **version** so gnu/llvm and version bumps
  don’t collide; include `hashFiles('tools/crosstools/**')` so patches to the
  toolchain recipe invalidate it.
* Keep the existing artifact publish so a) Stage B/C can `download:` it and b)
  the SourceForge `Developer_Tools` upload still happens when
  `arosbuild.withtoolchain=yes`.

### 5.4 Replace the `AROS.precontrib` full copy with a manifest

Today (`azure-pipelines.yml:659,677`) Stage 14 copies the whole built `AROS` tree
to `AROS.precontrib` and later walks it recursively to find contrib‑added files.
Replace that with a tiny text manifest.

**Exact‑equivalent method (recommended) — base manifest from Stage B.**
In `steps-core.yml`, after the base distribution is assembled, publish a manifest
listing every file in the base tree:

```bash
# Stage B (steps-core.yml): emit the authoritative base file list
# (files + symlinks; LC_ALL=C so the order matches the contrib-stage comm)
( cd "$(AROSBUILDDIR)/bin/$(arosbuild.name)/AROS" && find . \( -type f -o -type l \) | LC_ALL=C sort ) \
    > "$(AROSBUILDBINDIR)/$(AROSBUILDID)-$(arosbuild.name)-base.manifest"
# (published inside the distfiles-<name> artifact)
```

Then in `steps-contrib.yml`, contrib = everything present after the contrib build
that is **not** in the base manifest — byte‑for‑byte the same selection the
current full‑tree diff makes, for the price of a few KB:

```bash
AROSROOT="$(AROSBUILDDIR)/bin/$(arosbuild.name)/AROS"
download base.manifest from the distfiles-$(arosbuild.name) artifact   # → /tmp/base.lst
execute make -j $(BUILDTHREADS) contrib
( cd "$AROSROOT" && find . \( -type f -o -type l \) | LC_ALL=C sort ) > /tmp/postcontrib.lst
# run comm under the SAME locale the lists were sorted in, else it mis-diffs
LC_ALL=C comm -13 /tmp/base.lst /tmp/postcontrib.lst | while IFS= read -r f; do
  rel="${f#./}"
  dest="$(AROSBUILDSDIR)/$(AROSBUILDID)-$(arosbuild.name)-contrib/$rel"
  mkdir -p "${dest%/*}"          # ${...%/*} avoids the $(dirname ..) Azure-macro clash
  cp "$AROSROOT/$rel" "$dest"    # plain cp dereferences symlinks, matching the original
done
```

**Simpler fallback (no cross‑stage manifest) — local snapshot.** If you’d rather
not pass the manifest, snapshot the tree locally *after* `includes`+`linklibs`
but *before* contrib:

```bash
( cd "$AROSROOT" && find . \( -type f -o -type l \) | LC_ALL=C sort ) > /tmp/precontrib.lst
execute make -j $(BUILDTHREADS) contrib
( cd "$AROSROOT" && find . \( -type f -o -type l \) | LC_ALL=C sort ) > /tmp/postcontrib.lst
LC_ALL=C comm -13 /tmp/precontrib.lst /tmp/postcontrib.lst > /tmp/contrib-new.lst   # …copy as above
```

This is simpler but can *over‑include* a file that a full base build would have
placed outside `includes`/`linklibs` (e.g. a component contrib happens to pull in
that also ships in the base). For nightly parity the base‑manifest method avoids
that. Either way the second full copy of the SDK tree (and the recursive diff
function at `azure-pipelines.yml:677`) is gone.

Two correctness notes baked into the snippets above (verified against the old
`copycontribrecursive` on a tree with nested dirs, an in‑place‑modified base file
and a contrib symlink — identical file sets): include symlinks with
`\( -type f -o -type l \)` (the old walk packaged them; `-type f` alone would drop
them), and run **both** `sort` and `comm` under `LC_ALL=C` (the lists are
generated on two different agents, so a default‑locale `comm` over `C`‑sorted
input mis‑diffs). Modified‑in‑place files are intentionally **not** in the contrib
package (the original used path‑existence too); to catch them, snapshot
`find … -printf '%P %T@\n'` and diff on mtime as well.

### 5.5 Relocate the workspace to the largest volume (single‑agent relief)

Right after dependency install, pick the mount with the most free space and put
`AZBUILDPATH` there. Cheap insurance regardless of staging:

```bash
df -H
BEST=$(df -P /mnt "$AGENT_TEMPDIRECTORY" "$HOME" 2>/dev/null \
       | awk 'NR>1{print $4" "$6}' | sort -rh | head -1 | awk '{print $2}')
mkdir -p "$BEST/aros-build"
echo "##vso[task.setvariable variable=AZBUILDPATH;]$BEST/aros-build"
```

(On images where `/mnt` exists and is larger, this alone can rescue a build; on
images where it doesn’t, it’s a no‑op that picks the OS disk. Always confirm with
the `df -H` already printed at `azure-pipelines.yml:84`.)

### 5.6 Optional helper for symmetry with local builds

A thin wrapper lets a developer reproduce a single stage locally and keeps the
YAML steps tiny:

```bash
# scripts/azure/aros-stage.sh <toolchain|core|contrib|deploy>
#   uses $AROSSRCDIR, $AROSBUILDDIR, $AROSBUILDTOOLCHAINDIR, $AROSCONFIGOPTIONS,
#   $BUILDTHREADS from the environment (set by the pipeline or by hand locally).
set -euo pipefail
case "$1" in
  toolchain) "$AROSSRCDIR/configure" $AROSCONFIGOPTIONS --with-aros-toolchain-install="$AROSBUILDTOOLCHAINDIR"
             make -j"$BUILDTHREADS" crosstools ;;
  core)      "$AROSSRCDIR/configure" $AROSCONFIGOPTIONS --with-aros-toolchain-install="$AROSBUILDTOOLCHAINDIR" --with-aros-toolchain=yes
             make -j"$BUILDTHREADS"
             make -j"$BUILDTHREADS" boot-distfiles
             make -j"$BUILDTHREADS" distfiles ;;
  contrib)   "$AROSSRCDIR/configure" $AROSCONFIGOPTIONS --with-aros-toolchain-install="$AROSBUILDTOOLCHAINDIR" --with-aros-toolchain=yes
             make -j"$BUILDTHREADS" contrib ;;
esac
```

Local all‑in‑one users ignore this entirely and just run `make`.

---

## 6. Step‑by‑step Azure DevOps setup

### 6.1 One‑time project setup (already mostly in place)

1. **Secure file** `sf-azure-key` (the SourceForge SSH key) — Pipelines →
   Library → Secure files. Used by every stage that deploys.
2. **Variable group** (or pipeline secret variables) holding
   `SF_RSYNC_USER` and `SF_RSYNC_PASSWORD`. Link it to each flavour pipeline.
3. **Enable artifact caching** — `Cache@2` works out‑of‑the‑box on
   Microsoft‑hosted agents; no extra setup beyond the task in §5.2.
4. **Artifact retention** — set a short retention (e.g. 2–3 days) on the
   intermediate `toolchain-*`/`distfiles-*`/`contrib-*` pipeline artifacts so they
   don’t accumulate; the real distribution lives on SourceForge.

### 6.2 Per‑flavour pipeline (one per `arosbuild.name`, as today)

Each flavour remains its own pipeline pointing at
`scripts/azure/azure-pipelines.yml`. Keep the existing variables — they now drive
stages instead of steps:

| Variable | Example | Notes |
|----------|---------|-------|
| `arosbuild.name` | `pc-x86_64-smp` | artifact suffix |
| `arosbuild.target` | `pc-x86_64` | `--target=` |
| `arosbuild.variant` | `smp` | `--enable-target-variant=` |
| `arosbuild.vmimage` | `ubuntu-latest` | Microsoft‑hosted pool |
| `arosbuild.package` / `.packagefmt` | `boot-iso` / `lha` | base package |
| `arosbuild.bootpackage` / `.bootpackagefmt` | … | optional boot package |
| `arosbuild.toolchain.family` | `gnu` or `llvm` | drives Stage A + cache key |
| `arosbuild.gcc.version` / `.llvm.version` | e.g. `13.2.0` / `19.1.0` | cache key |
| `arosbuild.withtoolchain` | `yes` | also publish toolchain to SF |
| `arosbuild.withcontrib` | `yes` | enables Stage C |
| `arosbuild.withunittests` | `yes` | enables CUnit in Stage D |
| `arosbuild.deploy` | unset / `no` | gate SF upload (use `no` for test runs) |

No new per‑flavour variables are required for the basic staged build; the cache
and stage gating are derived from the values above.

### 6.3 Rollout

1. **Branch first.** Land the templated, staged YAML on a topic branch and point a
   **single throwaway flavour pipeline** (e.g. `pc-x86_64`, `arosbuild.deploy=no`)
   at it. Verify each stage’s disk with the `df -H` output.
2. Confirm **cache‑miss** Stage A builds the toolchain and **cache‑hit** on the
   next run skips it.
3. Confirm **Stage B** builds core + distfiles with the downloaded toolchain, and
   **Stage C** builds contrib self‑contained (watch `contrib-gnu`).
4. Confirm **Stage D** rsyncs the same file layout to `uploads/nightly2/` as
   today (the downstream `update-nightlies.yml` job is unchanged).
5. Flip the remaining flavour pipelines to the branch, then merge.

### 6.4 Self‑hosted note

The `vmImage` variable means these are Microsoft‑hosted agents, so “fresh agent =
fresh disk” holds for every stage. If you later move to **self‑hosted** agents,
staging still bounds the peak *provided each stage cleans up its work tree*
(`rm -Rf $(AROSBUILDDIR)/*` at the end, as Stage A already does) and you enable
**“Clean” = all** on the pipeline so the work dir is reset between runs.

---

## 7. Validation checklist

- [ ] `./configure && make && make contrib` still works locally (no build‑system diffs).
- [ ] Stage A cache miss builds toolchain; artifact published; `df -H` peak < agent disk.
- [ ] Stage A cache hit: `make crosstools` skipped (log says “restored from cache”).
- [ ] Stage B downloads toolchain to the **same abs path**; core + distfiles build; base/boot packaged.
- [ ] Stage C builds contrib **without** the core build tree; `contrib-gnu` completes; manifest diff produces the same contrib file set as the old recursive diff.
- [ ] Stage D uploads the identical directory layout to SourceForge; unit tests run when enabled.
- [ ] End‑to‑end nightly produces byte‑equivalent distribution archives vs. the legacy single‑job run (spot‑check md5 lists / file inventory).

## 8. Risks & mitigations

| Risk | Mitigation |
|------|-----------|
| Toolchain not relocatable (abs paths baked in) | Always restore to the same `Build.BinariesDirectory/toolchain` path and re‑run `configure --with-aros-toolchain-install=` in B/C (§4.2). |
| Cache key too loose → stale toolchain | Include family + version + `hashFiles('tools/crosstools/**')` (§5.3). |
| Contrib recompiles includes/linklibs each run (extra CPU) | Accepted trade vs. disk; 360‑min timeout is ample. If it ever binds, switch that flavour to Option B (§4.8). |
| Manifest diff misses in‑place‑modified files | Rare in contrib; extend the snapshot to include mtime if needed (§5.4). |
| Parallel B+C exhaust the agent **pool** (not disk) | Run B then C serially (`Contrib.dependsOn: Core`) on constrained pools. |
| `/mnt` absent on some images | §5.5 picker falls back to the OS disk automatically. |

---

## 9. Appendix — primitives reference

| Need | Command / target | Source |
|------|------------------|--------|
| Build toolchain into DIR | `configure --with-aros-toolchain-install=DIR; make crosstools` | `configure.in:2500`, `Makefile.in:180` |
| Reuse prebuilt toolchain | `configure --with-aros-toolchain=yes --with-aros-toolchain-install=DIR` | `configure.in:2381` |
| Share port downloads | `--with-portssources=DIR` | `configure.in:2923` |
| Core build | `make` (= `AROS.AROS`) | `Makefile.in:125` |
| Boot/main distfiles | `make boot-distfiles` / `make distfiles` | `arch/mmakefile.src:4‑8` |
| Contrib (all) | `make contrib` (deps: `includes linklibs`) | `config/mmakefile.src:19` |
| Contrib by category | `make contrib-<cat>` / `contrib-<cat>-clean` | contrib `mmakefile.src` |
| LLVM component clean (built‑in) | per‑component `make clean` | `tools/crosstools/llvm/mmakefile.src:503‑539` |
```
