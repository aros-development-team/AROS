/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm VOID MUI_RejectIDCMP(register __a0 Object *obj, register __d0 ULONG flags)
#else
	AROS_LH2(VOID, MUI_RejectIDCMP,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 16, MUIMaster)
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

    if (muiRenderInfo(obj) && (_flags(obj) & MADF_SETUP) && _win(obj))
    {
	if (muiAreaData(obj)->mad_hiehn.ehn_Events) DoMethod(_win(obj),MUIM_Window_RemEventHandler, &muiAreaData(obj)->mad_hiehn);
	muiAreaData(obj)->mad_hiehn.ehn_Events &= ~flags;
	if (muiAreaData(obj)->mad_hiehn.ehn_Events) DoMethod(_win(obj),MUIM_Window_AddEventHandler, &muiAreaData(obj)->mad_hiehn);
    } else muiAreaData(obj)->mad_hiehn.ehn_Events &= ~flags;

    AROS_LIBFUNC_EXIT

} /* MUIA_RejectIDCMP */
