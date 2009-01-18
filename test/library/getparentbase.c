/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <exec/libraries.h>
#include <aros/libcall.h>

#include LC_LIBDEFS_FILE

AROS_LH0(struct Library *, GetParentBase,
         struct Library *, PeridBase, 5, Perid
)
{
    AROS_LIBFUNC_INIT
    
    return GM_GETPARENTBASEID2(PeridBase);
    
    AROS_LIBFUNC_EXIT
}
