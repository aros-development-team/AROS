/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <intuition/preferences.h>
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(struct Preferences *, GetPrefs,

         /*  SYNOPSIS */
         AROS_LHA(struct Preferences * , prefbuffer, A0),
         AROS_LHA(WORD                 , size, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 22, Intuition)

/*  FUNCTION
    Gets a copy of the current Preferences structure.
 
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
    GetDefPrefs(), SetPrefs()
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_GETPREFS(dprintf("GetPrefs: Buffer 0x%lx Size 0x%lx Inform %d\n",
                           prefbuffer, size));

    if (prefbuffer != NULL && size != 0)
    {
        ULONG lock = LockIBase(0);
	
        CopyMem(GetPrivIBase(IntuitionBase)->ActivePreferences,
                prefbuffer,
                size <= sizeof(struct Preferences) ? size : sizeof(struct Preferences));
		
        UnlockIBase(lock);
    }

    return (struct Preferences *)prefbuffer;

    AROS_LIBFUNC_EXIT
} /* GetPrefs */
