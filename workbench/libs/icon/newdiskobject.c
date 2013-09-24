/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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
	Creates an empty DiskObject structure.

    INPUTS
	type - WBDISK, WBDRAWER, WBTOOL, WBPROJECT,
	       WBGARBAGE, WBDEVICE or WBKICK

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    APTR result = NULL;
    struct NativeIcon *ni;
    UWORD i;

    ni = (struct NativeIcon *)AllocMem(sizeof(struct NativeIcon), MEMF_CLEAR);
    if (ni == NULL)
        return NULL;

    ni->ni_DiskObject.do_Type = type;

    NEWLIST(&ni->ni_FreeList.fl_MemList);
    for (i = 0; i < 2; i++)
        ni->ni_Image[i].TransparentColor = -1;

    if (AddFreeList(&ni->ni_FreeList, ni, sizeof(struct NativeIcon)))
    {
        AddIconToList(ni, IconBase);
        result = &ni->ni_DiskObject;
    }
    else
        FreeMem(ni, sizeof(struct NativeIcon));

    return result;

    AROS_LIBFUNC_EXIT
}
