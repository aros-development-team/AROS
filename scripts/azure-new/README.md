# scripts/azure-new — staged AROS nightly pipeline

This directory is a **staged** re-implementation of the nightly Azure pipeline in
`scripts/azure/`. It exists **alongside** the original, which is left completely
untouched, so nothing breaks: you opt in by pointing an Azure pipeline at
`scripts/azure-new/azure-pipelines.yml` instead of `scripts/azure/azure-pipelines.yml`.

The motivation, root-cause analysis and full design rationale are in the
companion document **`aros-nightly-staged-build.md`** (kept with the build
checkout). This README is the quick operational reference.

## Why

The original pipeline runs the **whole** build (cross toolchain → core →
distfiles → contrib → deploy) as **one job on one agent**, so every intermediate
tree shares one storage-constrained disk. That is why:

* the **LLVM/Clang toolchain** runs out of space (and is rebuilt every night), and
* **GNU contrib** runs out of space (it is built on top of core, *plus* a full
  duplicate `AROS.precontrib`, *plus* the base/boot distfiles, all at once).

This pipeline splits the work into dependent **stages on fresh agents** and hands
only the toolchain forward, so no agent holds more than one heavy tree.

## Layout

```
azure-pipelines.yml              orchestrator: defines the stages
templates/
  steps-prepare-host.yml         SSH key, toolchain family, deps, thread count
  steps-darwin-hosttoolchain.yml native host GCC (macOS only; no-op on Linux)
  steps-workspace.yml            dir tree + build id (+ optional contrib clone)
  steps-source-package.yml       Sources stage body
  steps-toolchain.yml            Toolchain stage body (cache-aware)
  steps-core.yml                 Core stage body (core + distfiles + unittests)
  steps-unittests.yml            hosted CUnit testing (used by Core)
  steps-contrib.yml              Contrib stage body (self-contained)
aros-stage.sh                    run a single stage locally (developer convenience)
```

## Stage graph

| Stage | dependsOn | Builds | Hands forward |
|-------|-----------|--------|---------------|
| **Sources** | – | source + contrib-source tarballs | `sources-<name>` artifact + SF upload |
| **Toolchain** | – | `make crosstools` (cached across runs) | `toolchain-<name>` artifact (+ SF if `withtoolchain`) |
| **Core** | Toolchain | `make` + `boot-distfiles` + `distfiles`, base/boot packages, unit tests | `distfiles-<name>` + `base-manifest-<name>` artifacts + SF upload |
| **Contrib** | Toolchain, Core | `make contrib` (only `includes`+`linklibs`+contrib) | `contrib-<name>` artifact + SF upload |

`Sources` and `Toolchain` run in parallel; `Core` follows `Toolchain`; `Contrib`
follows both (it needs the toolchain artifact and the Core base manifest).

## Key mechanisms

* **Toolchain caching (`Cache@2`).** The toolchain is restored from a cache keyed
  on `arosbuild.name | family | gcc.version | llvm.version | hashFiles('tools/crosstools/**')`.
  On a hit, `make crosstools` is skipped entirely — **LLVM is not rebuilt every
  night**. On a miss, the LLVM tree is the only large thing on a fresh agent.
* **Toolchain is not relocatable.** `configure` bakes the absolute
  `CROSSTOOLSDIR` into the generated config, and GCC bakes its `--prefix`. The
  toolchain is always restored to the **same** absolute path
  (`$(Build.BinariesDirectory)/toolchain`) and `configure` is re-run pointing at
  it in the Core/Contrib stages.
* **Contrib packaging without the second tree copy.** The Core stage publishes a
  tiny `*-base.manifest` (a `find` path list of the fully-built base tree). The
  Contrib stage packages exactly the files **not** in that manifest. This is the
  same selection the original `AROS`-vs-`AROS.precontrib` existence diff makes,
  for a few KB instead of a full duplicate of the SDK tree.

## Pointing an Azure pipeline at this

1. **Reuse the existing project setup** unchanged: the `sf-azure-key` secure
   file and the `SF_RSYNC_USER` / `SF_RSYNC_PASSWORD` variables.
2. Create (or clone) a per-flavour pipeline and set its **YAML path** to
   `scripts/azure-new/azure-pipelines.yml`.
3. Keep the **same** `arosbuild.*` variables you already use — they now drive
   stages instead of steps. The relevant gates:
   * `arosbuild.withcontrib=yes` → enables the **Contrib** stage.
   * `arosbuild.withtoolchain=yes` → also uploads the toolchain to SourceForge.
   * `arosbuild.withunittests=yes` → runs CUnit in the **Core** stage.
   * `arosbuild.deploy=no` → build without uploading (use for test runs).
4. *(Recommended)* enable pipeline **artifact retention** of only a few days for
   the intermediate `toolchain-*` / `distfiles-*` / `contrib-*` / `base-manifest-*`
   artifacts; the real distribution lives on SourceForge.

The downstream jobs (`scripts/azure/update-nightlies.yml`, `gen-unittests.yml`,
…) are unchanged: this pipeline uploads the same `uploads/nightly2/` layout.

## Scope / caveats

* **Target: Linux Microsoft-hosted agents**, where the disk failures occur. The
  macOS dependency and flag branches are preserved so a Darwin flavour still
  works, but because each stage runs on a fresh agent the native host GCC would
  be rebuilt per stage — **macOS native-toolchain flavours are better kept on the
  original `scripts/azure` single-job pipeline.**
* `Contrib` recompiles `includes`+`linklibs` (not the full core). That trades a
  bit of CPU for bounded disk; the 360-minute job timeout is ample. If contrib
  recompile time ever becomes the constraint, switch that flavour to the
  "pass the build tree" variant described in the design doc (§4.8).

## Local builds are unaffected

Developers still build all-in-one exactly as before:

```sh
./configure [options] && make && make contrib
```

`aros-stage.sh` is only a convenience to reproduce one pipeline stage locally
(sharing a prebuilt toolchain + ports sources); it changes nothing in the build
system.
