/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <workbench/icon.h>
#include <proto/icon.h>

#include "icon_intern.h"

#   include <aros/debug.h>

/*****************************************************************************

    NAME */

    AROS_LH1(struct DiskObject *, NewDiskObject,

/*  SYNOPSIS */
        AROS_LHA(ULONG, type, D0),
        
/*  LOCATION */
        struct IconBase *, IconBase, 29, Icon)

/*  FUNCTION
	Create empty diskobject structure.
	
    INPUTS
	type - WBDISK, WBDRAWER, WBTOOL, WBPROJECT,
	       WBGARBAGE, WBDEVICE or WBKICK

    RESULT

    NOTES
	Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    int i;
    APTR pool;
    struct NativeIcon *ni;
    
    pool = CreatePool(0,1024,1024);
    if (!pool) return NULL;

    /* AROS doesn't need the gfx to be placed in chip niory, so we can use pools */
    ni = (struct NativeIcon *)AllocPooled(pool, sizeof(struct NativeIcon));
    if (!ni) return NULL;

    memset(ni, 0, sizeof(*ni));

    ni->ni_DiskObject.do_Type = type;
    
    NEWLIST(&ni->ni_FreeList.fl_MemList);
    for (i = 0; i < 2; i++) {
        ni->ni_Extra.Offset[i].IMAG = -1;
        ni->ni_Extra.Offset[i].PNG = -1;
        ni->ni_Image[i].TransparentColor = -1;
    }

    AddIconToList(ni, IconBase);

    return &ni->ni_DiskObject;
    
    AROS_LIBFUNC_EXIT
} /* NewDiskObject() */
