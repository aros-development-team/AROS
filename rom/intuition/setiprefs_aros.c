/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <intuition/iprefs.h>

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH3(ULONG, SetIPrefs,

         /*  SYNOPSIS */
         AROS_LHA(APTR , data, A0),
         AROS_LHA(ULONG, length, D0),
         AROS_LHA(ULONG, type, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 96, Intuition)

/*  FUNCTION

    INPUTS

    RESULT
    Depending on the operation

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ULONG Result = TRUE;
    ULONG lock = LockIBase(0);

    DEBUG_SETIPREFS(bug("SetIPrefs: data %p length %lu type %lu\n", data, length, type));

    switch (type)
    {
	case IPREFS_TYPE_ICONTROL:
            DEBUG_SETIPREFS(bug("SetIPrefs: IPREFS_TYPE_ICONTROL\n"));
            if (length > sizeof(struct IIControlPrefs))
        	length = sizeof(struct IIControlPrefs);
            CopyMem(data, &GetPrivIBase(IntuitionBase)->IControlPrefs, length);
            break;
        
	case IPREFS_TYPE_SCREENMODE:
            DEBUG_SETIPREFS(bug("SetIPrefs: IP_SCREENMODE\n"));
            if (length > sizeof(struct IScreenModePrefs))
                length = sizeof(struct IScreenModePrefs);
            CopyMem(data, &GetPrivIBase(IntuitionBase)->ScreenModePrefs, length);
            break;
	    
	default:
            DEBUG_SETIPREFS(bug("SetIPrefs: Unknown Prefs Type\n"));
            Result = FALSE;
            break;
    }

    UnlockIBase(lock);

    DEBUG_SETIPREFS(bug("SetIPrefs: Result 0x%lx\n",Result));
    
    return(Result);
    
    AROS_LIBFUNC_EXIT
} /* private1 */
