/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm VOID MUI_FreeClass(register __a0 struct IClass *classptr)
#else
	AROS_LH1(VOID, MUI_FreeClass,

/*  SYNOPSIS */
	AROS_LHA(struct IClass *, classptr, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 14, MUIMaster)
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

#ifndef __MAXON__
#warning FIXME: I should decrease the open count of library (use cl->hook->data)
#endif
    return;

    AROS_LIBFUNC_EXIT

} /* MUIA_FreeClass */
