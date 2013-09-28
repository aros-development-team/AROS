/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/arossupport.h>
#include "icon_intern.h"
#include <stddef.h>

extern const IPTR IconDesc[];

/*****************************************************************************

    NAME */
#include <clib/icon_protos.h>

	AROS_LH1(void, FreeDiskObject,

/*  SYNOPSIS */
	AROS_LHA(struct DiskObject *, diskobj, A0),

/*  LOCATION */
	struct IconBase *, IconBase, 15, Icon)

/*  FUNCTION
	Frees all memory for a DiskObject structure.
	
    INPUTS
	diskobj --  a pointer to a DiskObject structure. A NULL pointer will be
		    ignored.
	
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetDiskObject()
	
    INTERNALS

    HISTORY
	2006-04-09 Test for NULL pointer added
	
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if ( ! diskobj) return;

    struct NativeIcon *nativeicon;
    
    nativeicon = NATIVEICON(diskobj);
   
    /* Remove all layout specific data
     * (i.e. displayable bitmaps, pen allocations, etc.)
     */
    LayoutIconA(diskobj, NULL, NULL);

    RemoveIconFromList(nativeicon, LB(IconBase));

    /* It's enough to free our FreeList and the top-level structure */
    FreeFreeList(&nativeicon->ni_FreeList);
    FreeMem(nativeicon, sizeof(struct NativeIcon));

    AROS_LIBFUNC_EXIT
    
} /* FreeDiskObject */
