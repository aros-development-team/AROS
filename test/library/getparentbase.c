/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/pertask.h>
#include <aros/libcall.h>

#include LC_LIBDEFS_FILE

AROS_LH0(struct Library *, GetParentBase,
         struct PertaskBase *, PertaskBase, 5, Pertask
)
{
    AROS_LIBFUNC_INIT
    
    return (struct Library *)__GM_GetBaseParent(PertaskBase);
    
    AROS_LIBFUNC_EXIT
}

struct Library *GetParentBase2(void)
{
    struct PertaskBase *PertaskBase = __aros_getbase();

    return (struct Library *)__GM_GetBaseParent(PertaskBase);
}
