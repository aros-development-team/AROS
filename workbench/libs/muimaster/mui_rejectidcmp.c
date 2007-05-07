/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/intuition.h>
#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>

#include "muimaster_intern.h"
#include "mui.h"

/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_RejectIDCMP,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj  , A0),
	AROS_LHA(ULONG   , flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 16, MUIMaster)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (muiRenderInfo(obj) && (_flags(obj) & MADF_SETUP) && _win(obj))
    {
	if (muiAreaData(obj)->mad_hiehn.ehn_Events)
	{
	    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&muiAreaData(obj)->mad_hiehn);
	}
	muiAreaData(obj)->mad_hiehn.ehn_Events &= ~flags;
	if (muiAreaData(obj)->mad_hiehn.ehn_Events)
	{
	    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&muiAreaData(obj)->mad_hiehn);
	}
    }
    else
    {
	muiAreaData(obj)->mad_hiehn.ehn_Events &= ~flags;
    }

    AROS_LIBFUNC_EXIT

} /* MUIA_RejectIDCMP */
