/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hidd/graphics.h>

#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH1(void, UnLockBitMap,

/*  SYNOPSIS */
	AROS_LHA(APTR, Handle, A0),

/*  LOCATION */
	struct Library *, CyberGfxBase, 29, Cybergraphics)

/*  FUNCTION
        Releases exclusive access to a bitmap.

    INPUTS
        Handle - handle to the bitmap to unlock.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        UnLockBitMapTagList(), LockBitMapTagList()
    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (Handle)
        HIDD_BM_ReleaseDirectAccess((OOP_Object *)Handle);

    AROS_LIBFUNC_EXIT
} /* UnLockBitMap */
