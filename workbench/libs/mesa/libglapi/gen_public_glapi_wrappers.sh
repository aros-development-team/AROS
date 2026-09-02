#!/bin/sh

set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 <shared_glapi_mapi_tmp.h> <required_symbols.txt> <public_glapi_wrappers.c>" >&2
    exit 2
fi

in_header="$1"
required_symbols="$2"
out_c="$3"
tmp_c="${out_c}.tmp"

{
    echo "/* Auto-generated wrappers for required GL entrypoints. */"
    echo "#define MAPI_TMP_DEFINES"
    echo "#include \"shared_glapi_mapi_tmp.h\""
    echo
    awk '
        function trim(s) {
            gsub(/^[ \t]+|[ \t]+$/, "", s)
            return s
        }

        function arg_names(arglist,    n, i, part, name, res) {
            arglist = trim(arglist)
            if (arglist == "" || arglist == "void")
                return ""

            n = split(arglist, part, ",")
            res = ""
            for (i = 1; i <= n; i++) {
                part[i] = trim(part[i])
                gsub(/\[[^]]*\]/, "", part[i])
                if (match(part[i], /([A-Za-z_][A-Za-z0-9_]*)[ \t]*$/, m))
                    name = m[1]
                else
                    name = "arg" i
                res = res ((i > 1) ? ", " : "") name
            }
            return res
        }

        FNR == NR {
            sym = trim($0)
            if (sym ~ /^gl[A-Za-z0-9_]+$/ && !(sym in wanted)) {
                wanted[sym] = 1
                wanted_list[++wanted_count] = sym
            }
            next
        }

        /^[A-Za-z_][A-Za-z0-9_ \*]*GLAPIENTRY _dispatch_stub_[A-Za-z0-9_]+\(.*\);$/ {
            split($0, p1, " GLAPIENTRY _dispatch_stub_")
            if (length(p1) < 2)
                next

            ret = trim(p1[1])
            rest = p1[2]
            lparen = index(rest, "(")
            if (!lparen)
                next

            stub = substr(rest, 1, lparen - 1)
            args = substr(rest, lparen + 1, length(rest) - lparen - 2)
            stub_ret[stub] = ret
            stub_args[stub] = args
        }

        END {
            missing = 0
            for (i = 1; i <= wanted_count; i++) {
                glsym = wanted_list[i]
                stub = substr(glsym, 3)

                if (!(stub in stub_ret)) {
                    # Some core symbols map to ARB stubs and vice versa.
                    if (stub ~ /ARB$/) {
                        base = substr(stub, 1, length(stub) - 3)
                        if (base in stub_ret)
                            stub = base
                    } else {
                        arb = stub "ARB"
                        if (arb in stub_ret)
                            stub = arb
                    }
                }

                if (!(stub in stub_ret)) {
                    printf("Missing shared-glapi stub prototype for required symbol: %s\n", glsym) > "/dev/stderr"
                    missing = 1
                    continue
                }

                ret = stub_ret[stub]
                args = stub_args[stub]
                call_args = arg_names(args)

                printf("%s GLAPIENTRY %s(%s)\n", ret, glsym, args)
                print "{"
                if (ret == "void") {
                    if (call_args == "")
                        printf("   _dispatch_stub_%s();\n", stub)
                    else
                        printf("   _dispatch_stub_%s(%s);\n", stub, call_args)
                } else {
                    if (call_args == "")
                        printf("   return _dispatch_stub_%s();\n", stub)
                    else
                        printf("   return _dispatch_stub_%s(%s);\n", stub, call_args)
                }
                print "}"
                print ""
            }

            if (missing)
                exit 1
        }
    ' "$required_symbols" "$in_header"
} > "$tmp_c"

mv -f "$tmp_c" "$out_c"
