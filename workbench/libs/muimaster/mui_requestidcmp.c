/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <clib/alib_protos.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include <intuition/classusr.h>

#include "muimaster_intern.h"
#include "mui.h"

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm VOID MUI_RequestIDCMP(register __a0 Object *obj, register __d0 ULONG flags)
#else
	AROS_LH2(VOID, MUI_RequestIDCMP,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 15, MUIMaster)
#endif
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
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

/* Orginal idea: Get window object
 * Get list of objects that requested idcmp
 * put object and idcmp in list
 * window will check that list for matching idcmp
 */

    /* Use the Eventhandler to simulate an MUIM_HandleInput */
    if (muiRenderInfo(obj) && (_flags(obj) & MADF_SETUP) && _win(obj))
    {
	if (muiAreaData(obj)->mad_hiehn.ehn_Events) DoMethod(_win(obj),MUIM_Window_RemEventHandler, &muiAreaData(obj)->mad_hiehn);
	muiAreaData(obj)->mad_hiehn.ehn_Events |= flags;
	if (muiAreaData(obj)->mad_hiehn.ehn_Events) DoMethod(_win(obj),MUIM_Window_AddEventHandler, &muiAreaData(obj)->mad_hiehn);
    } else muiAreaData(obj)->mad_hiehn.ehn_Events |= flags;

    AROS_LIBFUNC_EXIT

} /* MUIA_RequestIDCMP */
