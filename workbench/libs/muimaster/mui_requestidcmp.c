/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <clib/alib_protos.h>
#include <proto/muimaster.h>

#include <proto/intuition.h>
#include <intuition/classusr.h>

#include "muimaster_intern.h"
#include "mui.h"

/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_RequestIDCMP,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj  , A0),
	AROS_LHA(ULONG   , flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 15, MUIMaster)

/*  FUNCTION

    For custom class implementors. Valid between MUIM_Setup/MUIM_Cleanup.
    Indicates that this object wants to receive some events. MUIM_HandleInput
    will then be called on this object whenever there's a matching event.

    INPUTS

    obj   -- an object whose class is derived from Area
    flags -- IDCMP flags

    RESULT

    NOTES

    Deprecated in favor of MUIM_Window_AddEventHandler and MUIM_HandleEvent
    event handling model (introduced in MUI V16).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    This is a wrapper around MUIM_Window_RemEventHandler and
    MUIM_Window_AddEventHandler.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Use the Eventhandler to simulate a MUIM_HandleInput */
    if (muiRenderInfo(obj) && (_flags(obj) & MADF_SETUP) && _win(obj))
    {
	if (muiAreaData(obj)->mad_hiehn.ehn_Events)
	{
	    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&muiAreaData(obj)->mad_hiehn);
	}
	muiAreaData(obj)->mad_hiehn.ehn_Events |= flags;
	if (muiAreaData(obj)->mad_hiehn.ehn_Events)
	{
	    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&muiAreaData(obj)->mad_hiehn);
	}
    }
    else
    {
	muiAreaData(obj)->mad_hiehn.ehn_Events |= flags;
    }

    AROS_LIBFUNC_EXIT

} /* MUIA_RequestIDCMP */
