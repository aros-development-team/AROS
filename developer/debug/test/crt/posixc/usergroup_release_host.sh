#!/bin/sh
# Build and run usergroup_release_host.c on the host.
#   usergroup_release_host.sh <AROS source tree> [compiler [compiler-arg...]]
# The compiler is the command itself followed by any of its own arguments, one
# word per argument, for example:
#   usergroup_release_host.sh ~/AROS cc -fsanitize=address,undefined
# It defaults to cc; none of its arguments may be --. CC in the environment
# is not used.
# posixc's __optionallibs.c and stdc's __exitfunc.c are compiled unchanged.
# Each is reached through a symlink in a scratch directory, so that its own
# quoted includes resolve to the stand-ins in usergroup_release_host/ rather
# than to the private headers next to the real file.
set -e
SRC=${1:?usage: $0 <AROS source tree> [compiler [compiler-arg...]]}
shift
[ $# -gt 0 ] || set -- cc
HERE=$(cd "$(dirname "$0")" && pwd)
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
ln -s "$SRC/compiler/crt/posixc/__optionallibs.c" "$W/__optionallibs.c"
ln -s "$SRC/compiler/crt/stdc/__exitfunc.c" "$W/__exitfunc.c"
MOCK="$HERE/usergroup_release_host"

# compile <compiler...> -- <arguments...>
# Runs the compiler with the common flags in place of the --, so that they come
# before this call's own arguments and a call's -Wno-* options take effect.
compile() {
    for a do
        shift
        if [ "$a" = -- ]; then
            set -- "$@" -std=gnu99 -O2 -Wall -Wextra -Werror \
                -iquote "$MOCK" -I "$MOCK" \
                -iquote "$SRC/compiler/crt/posixc" -iquote "$SRC/compiler/crt/stdc"
        else
            set -- "$@" "$a"
        fi
    done
    "$@"
}

# The -Wno-* flags cover warnings that already exist in the production files
# and are not introduced by this test: __libfindandopen is unused, and
# __optionallibs_close has no return statement.
compile "$@" -- -Wno-unused-function -Wno-return-type -Wno-unused-parameter \
    -Dmalloc=test_malloc -c "$W/__optionallibs.c" -o "$W/optionallibs.o"
compile "$@" -- -Wno-unused-function -Wno-return-type -Wno-unused-parameter \
    -c "$W/__exitfunc.c" -o "$W/exitfunc.o"
compile "$@" -- -c "$HERE/usergroup_release_host.c" -o "$W/test.o"
"$@" -o "$W/usergroup_release_host" "$W/test.o" "$W/optionallibs.o" "$W/exitfunc.o"
"$W/usergroup_release_host"
