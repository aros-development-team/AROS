/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <intuition/preferences.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(struct Preferences *, GetDefPrefs,

         /*  SYNOPSIS */
         AROS_LHA(struct Preferences * , prefbuffer, A0),
         AROS_LHA(WORD                 , size, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 21, Intuition)

/*  FUNCTION
    Gets a copy of the Intuition default Preferences structure.
 
    INPUTS
    prefbuffer - The buffer which contains your settings for the
        preferences.
    size - The number of bytes of the buffer you want to be copied.
 
    RESULT
    Returns your parameter buffer.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    GetPrefs(), SetPrefs()
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_GETDEFPREFS(dprintf("GetDefPrefs: Buffer 0x%lx Size 0x%lx Inform %d\n",
                              prefbuffer, size));

    if (prefbuffer != NULL && size != 0)
    {
        ULONG lock = LockIBase(0);
	
        CopyMem(GetPrivIBase(IntuitionBase)->DefaultPreferences,
                prefbuffer,
                size <= sizeof(struct Preferences) ? size : sizeof(struct Preferences));
		
        UnlockIBase(lock);
    }

    return (struct Preferences *)prefbuffer;

    AROS_LIBFUNC_EXIT
} /* GetDefPrefs */
