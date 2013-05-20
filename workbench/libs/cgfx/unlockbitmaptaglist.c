/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH2(void, UnLockBitMapTagList,

/*  SYNOPSIS */
	AROS_LHA(APTR            , Handle, A0),
	AROS_LHA(struct TagItem *, Tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 30, Cybergraphics)

/*  FUNCTION
        Releases exclusive access to a bitmap. Options for the unlocking
        process are given in a taglist. The possible tags are as follows:
            UBMI_UPDATERECTS (struct RectList *) - pointer to a series of
                rectangle lists that need to be refreshed.
            UBMI_REALLYUNLOCK (BOOL) - if FALSE, the bitmap will not be
                unlocked; only rectangle updates will be done.

    INPUTS
        Handle - handle to the bitmap to unlock.
        Tags - a taglist as described above.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        UnLockBitMap(), LockBitMapTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct TagItem *tag;
    BOOL reallyunlock = TRUE;
    
    while ((tag = NextTagItem(&Tags)))
    {
    	switch (tag->ti_Tag)
	{
	    case UBMI_REALLYUNLOCK:
	    	reallyunlock = (BOOL)tag->ti_Data;
		break;
		
	    case UBMI_UPDATERECTS:
	    {
#if 0
	    	struct RectList *rl;
		
		rl = (struct RectList *)tag->ti_Data;
		
		/* TODO: Call HIDD_BM_UpdateRect() for all the regions */
#endif		
	    	break;
	    }
	
	    default:
	    	D(bug("!!! UNKNOWN TAG PASSED TO UnLockBitMapTagList() !!!\n"));
		break;
	}
    }
    
    if (reallyunlock)
    {
	HIDD_BM_ReleaseDirectAccess((OOP_Object *)Handle);
    }

    AROS_LIBFUNC_EXIT
} /* UnLockBitMapTagList */
