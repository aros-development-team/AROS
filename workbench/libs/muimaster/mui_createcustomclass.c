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
__asm struct MUI_CustomClass *MUI_CreateCustomClass(register __a0 struct Library *base, register __a1 char *supername, register __a2 struct MUI_CustomClass *supermcc,register __d0 int datasize,register __a3 APTR dispatcher)
#else
	AROS_LH5(struct MUI_CustomClass *, MUI_CreateCustomClass,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, base, A0),
	AROS_LHA(char *, supername, A1),
	AROS_LHA(struct MUI_CustomClass *, supermcc, A2),
	AROS_LHA(int, datasize, D0),
	AROS_LHA(APTR, dispatcher, A3),

/*  LOCATION */
	struct MUIMasterBase *, MUIMasterBase, 18, MUIMaster)
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

} /* MUIA_CreateCustomClass */
