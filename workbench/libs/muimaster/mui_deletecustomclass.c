/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm BOOL MUI_DeleteCustomClass(register __a0 struct MUI_CustomClass *mcc)
#else
	AROS_LH1(BOOL, MUI_DeleteCustomClass,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_CustomClass *, mcc, A0),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 19, MUIMaster)
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

    AROS_LIBFUNC_EXIT

} /* MUIA_DeleteCustomClass */
