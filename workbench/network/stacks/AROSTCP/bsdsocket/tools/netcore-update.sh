#!/bin/bash
#
# Copyright (C) 2026 The AROS Dev Team
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# netcore-update.sh - bring net/netcore.def and net/netcore_map.h in step
# with the sources as they are now.
#
#   netcore-update.sh [-a] [-e] [-k] [-w DIR] [-o OBJDIR] [-f "CFLAGS"] CROSSPREFIX
#
#   CROSSPREFIX  the AROS cross tools without the tool's name, e.g.
#                <build>/bin/linux-x86_64/tools/crosstools/x86_64-aros-
#                (m68k: .../m68k-aros- with -f "-Os -m68000")
#   -a           append the names that are missing to net/netcore.def.
#                Without it the script only reports. After an append:
#                raise NETCORE_VERSION in net/netcore.h.
#   -e           also list, file by file and line by line, what has to
#                change in the protocol sources before they can be built
#                into a module library (nothing is changed)
#   -w DIR       where the objects go (default: a new temporary directory;
#                nothing is written into a build directory)
#   -k           keep that directory
#   -f "CFLAGS"  compiler options for the target (default: -O2)
#   -o OBJDIR    for a source file that does not compile at this moment
#                (the tree is being worked on), use the object of the same
#                name from OBJDIR - the stack's directory under gen/ in a
#                build directory - so that what it uses is not lost from
#                the list. Read only.
#
# What it does:
#   1. compiles every source file of the four groups of stack.cfg (core,
#      transports, IPv4, IPv6 - the checksum files of every processor, of
#      which those written for another one do not compile and are left
#      out), and the module sources outside the stack program
#      (MODULE_EXTRA below), the ordinary way;
#   2. sorts the objects into core and module by the group they are in;
#   3. mkimports.py derive: every symbol a module object needs and a core
#      object defines belongs in net/netcore.def. Reports what is missing,
#      what stands in the way (a local declaration of a core name in a
#      module source, a variable named like a structure, a core function
#      in a static initialiser) and what a module takes from ANOTHER
#      module, which no table can give it;
#   4. mkimports.py map: writes net/netcore_map.h from the list;
#   5. compiles every module source with -DNETMODULE and checks that none
#      of them names the core any more (mkimports.py verify);
#   6. for each protocol library (transport, inet, inet6): lists the names
#      its objects take from ANOTHER library's objects. There must be
#      none - two library files are not linked with each other; what one
#      offers another goes through a named interface (struct netiface,
#      net/netmodule.h).
#
# No symbol is looked up by name when a library is loaded: this runs when
# the sources change, and what it writes is C source that is committed.
#
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
S=$(cd "$HERE/.." && pwd)                               # bsdsocket/
INC=$(cd "$S/../../../common/include" && pwd)           # the public network headers
DEF=$S/net/netcore.def
MAP=$S/net/netcore_map.h

# module sources that are not part of the stack program
MODULE_EXTRA="$S/../modules/demo/demo.c"

# a file list of stack.cfg, for every processor it selects files by
group() {
    for cpu in x86_64 aarch64 other; do
        printf 'include %s/stack.cfg\nshow: ; @echo $(%s)\n' "$S" "$1" |
            make -s -f - AROS_TARGET_CPU=$cpu SRCDIR=/nowhere show
    done | tr ' ' '\n' | grep . | sort -u | tr '\n' ' '
}
# the optional groups (conf/group-*.cfg) are in STACK_CORE_C while they are
# built into the program, but they are checked as what they will become:
# module objects
G_OPT="$(group STACK_RESOLVER_C) $(group STACK_ADMIN_C) $(group STACK_PACKET_C) $(group STACK_NETCONFIG_C)"
G_CORE=$(for f in $(group STACK_CORE_C); do case " $G_OPT " in *" $f "*) ;; *) echo $f;; esac; done | tr '\n' ' ')
G_TRANSPORT=$(group STACK_TRANSPORT_C)
G_INET=$(group STACK_INET_C)
G_INET6=$(group STACK_INET6_C)

APPEND=""; KEEP=0; W=""; TFLAGS="-O2"; OBJDIR=""; EDITS=0
while getopts "aekw:f:o:" o; do
    case $o in
    a) APPEND="--append";;
    e) EDITS=1;;
    k) KEEP=1;;
    w) W=$OPTARG;;
    f) TFLAGS=$OPTARG;;
    o) OBJDIR=$OPTARG;;
    *) sed -n '10,36p' "$0"; exit 2;;
    esac
done
shift $((OPTIND - 1))
[ $# -eq 1 ] || { sed -n '10,36p' "$0"; exit 2; }
X=$1
CC=${X}gcc; NM=${X}nm; OBJDUMP=${X}objdump
[ -x "$CC" ] || { echo "no compiler at $CC"; exit 2; }

if [ -z "$W" ]; then W=$(mktemp -d); else mkdir -p "$W"; KEEP=1; fi
mkdir -p "$W/core" "$W/proto" "$W/asmod"
rm -f "$W"/core/*.o "$W"/proto/*.o "$W"/asmod/*.o "$W"/*/*.err

# the options of mmakefile.src (USER_CPPFLAGS, USER_INCLUDES); the public
# headers are taken from the source tree, not from an installed copy
FLAGS="-c -std=gnu99 $TFLAGS -fno-common -I$INC -idirafter $S -idirafter $S/protos \
 -DDIRECTED_BROADCAST -DENABLE_MULTICAST -DENABLE_FDLIBRARY -DAMITCP -DKERNEL -DENABLE_SYSCTL \
 -D_XOPEN_SOURCE=600 -D__BSD_VISIBLE=1 -Wno-implicit-function-declaration -w"

compile() {     # source  object  extra options...
    local src=$1 obj=$2; shift 2
    "$CC" $FLAGS "$@" -iquote "$(dirname "$src")" -iquote "$S" "$src" -o "$obj" 2> "$obj.err" \
        && rm -f "$obj.err"
}

# a .c file of the protocol directories that is in no group is not built by anything
for f in $(cd "$S" && ls netinet/*.c netinet6/*.c | sed 's/\.c$//'); do
    case " $G_TRANSPORT $G_INET $G_INET6 " in *" $f "*) ;; *) echo "IN NO GROUP of stack.cfg: $f.c";; esac
done

# 1./2. the files of the four groups
MODSRC=""
for f in $G_CORE $G_TRANSPORT $G_INET $G_INET6 $G_OPT; do
    b=$(basename "$f")
    case " $G_CORE " in *" $f "*) d=core;; *) d=proto; MODSRC="$MODSRC $S/$f.c";; esac
    compile "$S/$f.c" "$W/$d/$b.o" &
    while [ "$(jobs -r | wc -l)" -ge 16 ]; do wait -n; done
done
for src in $MODULE_EXTRA; do
    MODSRC="$MODSRC $src"
    compile "$src" "$W/proto/$(basename "$src" .c).o" &
done
wait
# checksum routines written for another processor do not compile here, and are not wanted
for e in "$W"/core/*.err "$W"/proto/*.err; do
    [ -f "$e" ] || continue
    b=$(basename "$e" .o.err)
    if [ -n "$OBJDIR" ] && [ -f "$OBJDIR/$b.o" ]; then
        cp "$OBJDIR/$b.o" "${e%.err}"
        echo "DOES NOT COMPILE, the object from $OBJDIR is used instead: $b.c"
    else
        echo "DOES NOT COMPILE (left out): $b.c"
    fi
    grep -m2 error "$e" | cut -c1-200
done
echo "compiled: $(ls "$W"/core/*.o | wc -l) core files, $(ls "$W"/proto/*.o | wc -l) module files"

# 3.
python3 "$HERE/mkimports.py" derive --nm "$NM" --objdump "$OBJDUMP" \
    --core "$W"/core/*.o --module "$W"/proto/*.o \
    --def "$DEF" --inc "$INC" "$S" --src $MODSRC $APPEND || exit 1

# 4.
python3 "$HERE/mkimports.py" map --def "$DEF" -o "$MAP" || exit 1

if [ $EDITS = 1 ]; then
    echo "--- edits the protocol sources need before they can be built into a module library:"
    python3 "$HERE/mkimports.py" edits --def "$DEF" --src $MODSRC
fi

# 5. (a file that did not compile the ordinary way - another processor's - is not tried)
rc=0
for src in $MODSRC; do
    b=$(basename "$src" .c)
    [ -f "$W/proto/$b.o" ] || continue
    { compile "$src" "$W/asmod/$b.o" -DNETMODULE || {
        echo "DOES NOT COMPILE AS A MODULE: $src"; head -5 "$W/asmod/$b.o.err"; }; } &
    while [ "$(jobs -r | wc -l)" -ge 16 ]; do wait -n; done
done
wait
ls "$W"/asmod/*.err > /dev/null 2>&1 && rc=1
if ls "$W"/asmod/*.o > /dev/null 2>&1; then
    python3 "$HERE/mkimports.py" verify --nm "$NM" --core "$W"/core/*.o --module "$W"/asmod/*.o || rc=1
fi
# the core's table itself: every name in the list must exist with the type its header gives it
mkdir -p "$W/table"
compile "$S/kern/net_services.c" "$W/table/net_services.o" || {
    echo "kern/net_services.c DOES NOT COMPILE: a line of net/netcore.def names something the headers do not declare";
    grep -m5 error "$W/table/net_services.o.err" | cut -c1-240; rc=1; }

# 6. no library may take a name from another library
defs() { for f in "$@"; do [ -f "$W/asmod/$(basename $f).o" ] && "$NM" --defined-only "$W/asmod/$(basename $f).o"; done |
         awk 'NF == 3 && $2 ~ /^[TDBRCGSV]$/ { print $3 }' | sort -u; }
unds() { for f in "$@"; do [ -f "$W/asmod/$(basename $f).o" ] && "$NM" -u "$W/asmod/$(basename $f).o"; done |
         awk 'NF == 2 { print $2 }' | sort -u; }
defs $G_TRANSPORT > "$W/transport.def"; defs $G_INET > "$W/inet.def"; defs $G_INET6 > "$W/inet6.def"
for g in transport inet inet6; do
    case $g in transport) L=$G_TRANSPORT;; inet) L=$G_INET;; inet6) L=$G_INET6;; esac
    unds $L > "$W/$g.und"
    for o in transport inet inet6; do
        [ $o = $g ] && continue
        x=$(comm -12 "$W/$g.und" "$W/$o.def" | comm -23 - "$W/$g.def" | tr '\n' ' ')
        if [ -n "$x" ]; then echo "library $g takes from library $o: $x"; rc=1; fi
    done
done
echo "cross-library references checked: transport, inet, inet6"

if [ $KEEP = 1 ]; then echo "objects kept in $W"; else rm -rf -- "$W"; fi
exit $rc
