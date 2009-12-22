/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/
#include <exec/types.h>
#include <aros/libcall.h>

AROS_LH2I(ULONG, add,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct dummybase *,dummybase,5,Dummy
)
{
    AROS_LIBFUNC_INIT
    return a+b;
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(ULONG, __int_asl,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct dummybase *,dummybase,6,Dummy
)
{
    AROS_LIBFUNC_INIT
    return a<<b;
    AROS_LIBFUNC_EXIT
}
