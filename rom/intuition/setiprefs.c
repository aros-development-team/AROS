/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <prefs/icontrol.h>
#include <intuition/iprefs.h>

#include "intuition_intern.h"

#define DEBUG_SETIPREFS(x) ;

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
	{
	    struct IIControlPrefs *prefs = (struct IIControlPrefs *)data;
	    
            DEBUG_SETIPREFS(bug("SetIPrefs: IPREFS_TYPE_ICONTROL\n"));
            
	    GetPrivIBase(IntuitionBase)->MenusUnderMouse = (prefs->ic_Flags & ICF_POPUPMENUS) ? TRUE : FALSE;
	    GetPrivIBase(IntuitionBase)->MenuLook = (prefs->ic_Flags & ICF_3DMENUS) ? MENULOOK_3D : MENULOOK_CLASSIC;
            break;
    	}
	
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
