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
__asm struct IClass *MUI_GetClass(register __a0 char *classname)
#else
	AROS_LH1(struct IClass *, MUI_GetClass,

/*  SYNOPSIS */
	AROS_LHA(char *, classname, A0),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 13, MUIMaster)
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

} /* MUIA_GetClass */
