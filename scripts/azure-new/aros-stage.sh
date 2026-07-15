#!/bin/sh
#
# aros-stage.sh - run a single AROS build stage locally, the same way the staged
# Azure pipeline does. This is purely a convenience for reproducing one stage on
# a developer machine; the normal local "all in one" build is unaffected and is
# still just:
#
#     ./configure [options] && make && make contrib
#
# Usage:
#     scripts/azure-new/aros-stage.sh <toolchain|core|contrib> [configure-args...]
#
# Environment (set sane defaults if unset):
#     AROSSRCDIR              - AROS source tree (default: git toplevel)
#     AROSBUILDDIR            - build directory   (default: $PWD/build)
#     AROSBUILDTOOLCHAINDIR   - toolchain install (default: $PWD/toolchain)
#     AROSPORTSSRCSDIR        - shared port srcs  (default: $PWD/portssources)
#     BUILDTHREADS            - parallel jobs      (default: nproc)
#
# Typical local sequence (mirrors the pipeline, shared toolchain + ports):
#     export AROSBUILDTOOLCHAINDIR=$PWD/toolchain AROSPORTSSRCSDIR=$PWD/portssources
#     scripts/azure-new/aros-stage.sh toolchain --target=pc-x86_64
#     scripts/azure-new/aros-stage.sh core      --target=pc-x86_64
#     scripts/azure-new/aros-stage.sh contrib   --target=pc-x86_64
#
set -eu

stage="${1:-}"
[ -n "$stage" ] || { echo "usage: $0 <toolchain|core|contrib> [configure-args...]" >&2; exit 2; }
shift || true

: "${AROSSRCDIR:=$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
: "${AROSBUILDDIR:=$PWD/build}"
: "${AROSBUILDTOOLCHAINDIR:=$PWD/toolchain}"
: "${AROSPORTSSRCSDIR:=$PWD/portssources}"
: "${BUILDTHREADS:=$(nproc 2>/dev/null || echo 1)}"

mkdir -p "$AROSBUILDDIR" "$AROSBUILDTOOLCHAINDIR" "$AROSPORTSSRCSDIR"
cd "$AROSBUILDDIR"

COMMON="--enable-build-type=nightly --enable-ccache --with-portssources=$AROSPORTSSRCSDIR"

case "$stage" in
  toolchain)
    "$AROSSRCDIR/configure" $COMMON "$@" --with-aros-toolchain-install="$AROSBUILDTOOLCHAINDIR"
    make -j"$BUILDTHREADS" crosstools
    ;;
  core)
    "$AROSSRCDIR/configure" $COMMON "$@" \
        --with-aros-toolchain-install="$AROSBUILDTOOLCHAINDIR" --with-aros-toolchain=yes
    make -j"$BUILDTHREADS"
    make -j"$BUILDTHREADS" boot-distfiles
    make -j"$BUILDTHREADS" distfiles
    ;;
  contrib)
    [ -d "$AROSSRCDIR/contrib" ] || echo "note: place/clone contrib sources at $AROSSRCDIR/contrib first" >&2
    "$AROSSRCDIR/configure" $COMMON "$@" \
        --with-aros-toolchain-install="$AROSBUILDTOOLCHAINDIR" --with-aros-toolchain=yes
    make -j"$BUILDTHREADS" contrib
    ;;
  *)
    echo "unknown stage '$stage' (expected toolchain|core|contrib)" >&2
    exit 2
    ;;
esac
