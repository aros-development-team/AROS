/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <exec/libraries.h>
#include <aros/libcall.h>

#include LC_LIBDEFS_FILE

AROS_LH0(struct Library *, GetParentBase,
         struct Library *, PertaskBase, 5, Pertask
)
{
    AROS_LIBFUNC_INIT
    
    return __GM_GetBaseParent(PertaskBase);
    
    AROS_LIBFUNC_EXIT
}
