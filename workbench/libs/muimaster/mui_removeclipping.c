/*
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_RemoveClipping,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(APTR, handle, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 25, MUIMaster)

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
