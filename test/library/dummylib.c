/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/
#include <stdarg.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE

#include "dummybase.h"

AROS_LH2(ULONG, add,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct DummyBase *,DummyBase,5,Dummy
)
{
    AROS_LIBFUNC_INIT
    return (DummyBase->lastval = a+b);
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, __int_asl,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct DummyBase *,DummyBase,6,Dummy
)
{
    AROS_LIBFUNC_INIT
    return (DummyBase->lastval = a<<b);
    AROS_LIBFUNC_EXIT
}

LONG printx(LONG nargs, ...)
{
    struct DummyBase *DummyBase = __aros_getbase();
    struct Library *DOSBase = OpenLibrary("dos.library", 0);
    va_list args;
    LONG i;

    va_start(args, nargs); 

    if (DOSBase) {
        for (i = 0; i < nargs; i++) {
            Printf("\t%ld: %ld\n", i, DummyBase->lastval + va_arg(args, LONG));
        }
    }

    va_end(args);

    return nargs;
}
