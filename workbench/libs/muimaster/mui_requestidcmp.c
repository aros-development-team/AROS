/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include <intuition/classusr.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
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

/* Get window object
 * Get list of objects that requested idcmp
 * put object and idcmp in list
 * window will check that list for matching idcmp
 */

    AROS_LIBFUNC_EXIT

} /* MUIA_RequestIDCMP */
