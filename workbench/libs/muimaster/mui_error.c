/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include "muimaster_intern.h"
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm LONG MUI_Error(VOID)
#else
	AROS_LH0(LONG, MUI_Error,

/*  SYNOPSIS */

/*  LOCATION */
	struct Library *, MUIMasterBase, 11, MUIMaster)
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

    return 0;

    AROS_LIBFUNC_EXIT

} /* MUIA_Error */
