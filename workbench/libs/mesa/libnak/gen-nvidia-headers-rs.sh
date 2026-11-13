#!/bin/sh
# Generate the Rust side of mesa's nvidia class headers (the nvidia_headers
# crate), following src/nouveau/headers/meson.build.
#
# usage: gen-nvidia-headers-rs.sh <mesa src/nouveau/headers dir> <output dir> <python> <bindgen command, may start with VAR=value> -- <clang args>
set -e
H=$1; OUT=$2; PYTHON=$3; BINDGEN=$4; shift 4
[ "$1" = "--" ] && shift
CLANGARGS="$*"
mkdir -p "$OUT"
NVCL=$(sed -n "/^nv_classes = \[/,/^\]/p" "$H/meson.build" | grep -o "'cl[0-9a-f]*'" | tr -d "'")
HWREF=$(sed -n "/^hwref_gens = \[/,/^\]/p" "$H/meson.build" | grep -o "'[a-z]*/[a-z0-9]*'" | tr -d "'")
RSFILES=""
prev_of() {
    prev=""
    for p in $NVCL; do
        [ "$p" = "$1" ] && break
        [ "$(echo "$p" | tail -c 3)" = "$(echo "$1" | tail -c 3)" ] && prev=$p
    done
    echo "$prev"
}
for cl in $NVCL; do
    $PYTHON "$H/class_parser.py" --in-h "$H/nvidia/classes/$cl.h" --out-rs "$OUT/nvh_classes_$cl.rs"
    RSFILES="$RSFILES $OUT/nvh_classes_$cl.rs"
    prev=$(prev_of $cl); PARGS=""
    [ -n "$prev" ] && PARGS="--prev-in-h $H/nvidia/classes/$prev.h"
    $PYTHON "$H/class_parser.py" --in-h "$H/nvidia/classes/$cl.h" $PARGS --out-rs-mthd "$OUT/nvh_classes_${cl}_mthd.rs"
    RSFILES="$RSFILES $OUT/nvh_classes_${cl}_mthd.rs"
    case $cl in *c0)
        if [ -f "$H/nvidia/classes/${cl}qmd.h" ]; then
            $PYTHON "$H/struct_parser.py" --in-h "$H/nvidia/classes/${cl}qmd.h" --out-rs "$OUT/nvh_classes_${cl}_qmd.rs"
            RSFILES="$RSFILES $OUT/nvh_classes_${cl}_qmd.rs"
        fi ;;
    esac
    case $cl in *97)
        $PYTHON "$H/struct_parser.py" --in-h "$H/nvidia/classes/${cl}tex.h" --out-rs "$OUT/nvh_classes_${cl}_tex.rs"
        RSFILES="$RSFILES $OUT/nvh_classes_${cl}_tex.rs" ;;
    esac
    if [ -f "$H/nvidia/classes/${cl}sph.h" ]; then
        $PYTHON "$H/struct_parser.py" --in-h "$H/nvidia/classes/${cl}sph.h" --out-rs "$OUT/nvh_classes_${cl}_sph.rs"
        RSFILES="$RSFILES $OUT/nvh_classes_${cl}_sph.rs"
    fi
done
for fg in $HWREF; do
    gen=${fg#*/}
    # eval: the bindgen command carries quoted --raw-line arguments
    eval "env $BINDGEN --allowlist-var 'NV_MMU_.*' -o \"$OUT/nvh_hwref_${gen}_mmu.rs\" \"$H/nvidia/hwref/$fg/dev_mmu.h\" -- $CLANGARGS"
    RSFILES="$RSFILES $OUT/nvh_hwref_${gen}_mmu.rs"
done
$PYTHON "$H/lib_rs_gen.py" --out-rs "$OUT/lib.rs" $RSFILES
