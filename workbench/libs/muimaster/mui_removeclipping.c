/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm VOID MUI_RemoveClipping(register __a0 struct MUI_RenderInfo *mri, register __a1 APTR handle)
#else
	AROS_LH2(VOID, MUI_RemoveClipping,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(APTR, handle, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 25, MUIMaster)
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

    MUI_RemoveClipRegion(mri, handle);

    AROS_LIBFUNC_EXIT

} /* MUIA_RemoveClipping */
