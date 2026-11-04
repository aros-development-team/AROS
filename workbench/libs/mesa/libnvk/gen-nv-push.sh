#!/bin/sh
# Generate the C side of mesa's nvidia class headers (nv_push_<class>.h/.c
# for every class and nv_push_class_dump.h), following
# src/nouveau/headers/meson.build.
#
# usage: gen-nv-push.sh <mesa src/nouveau/headers dir> <output dir> <python>
set -e
H=$1; OUT=$2; PYTHON=$3
mkdir -p "$OUT"
NVCL=$(sed -n "/^nv_classes = \[/,/^\]/p" "$H/meson.build" | grep -o "'cl[0-9a-f]*'" | tr -d "'")
prev_of() {
    prev=""
    for p in $NVCL; do
        [ "$p" = "$1" ] && break
        [ "$(echo "$p" | tail -c 3)" = "$(echo "$1" | tail -c 3)" ] && prev=$p
    done
    echo "$prev"
}
for cl in $NVCL; do
    prev=$(prev_of $cl); PARGS=""
    [ -n "$prev" ] && PARGS="--prev-in-h $H/nvidia/classes/$prev.h"
    $PYTHON "$H/class_parser.py" --in-h "$H/nvidia/classes/$cl.h" $PARGS \
        --out-h "$OUT/nv_push_$cl.h" --out-c "$OUT/nv_push_$cl.c"
done
$PYTHON "$H/nv_push_class_dump_h.py" --classes "$(echo $NVCL | tr '\n' ' ')" --out-h "$OUT/nv_push_class_dump.h"
