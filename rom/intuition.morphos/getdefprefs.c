/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
 
    Desc: Intuition function GetDefPrefs()
    Lang: english
*/
#include <proto/exec.h>
#include "intuition_intern.h"
#include <intuition/preferences.h>

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
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_GETDEFPREFS(dprintf("GetDefPrefs: Buffer 0x%lx Size 0x%lx Inform %d\n",
                              prefbuffer, size));

    if (prefbuffer)
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
