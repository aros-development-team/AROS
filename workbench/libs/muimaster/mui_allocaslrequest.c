/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/asl.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm APTR MUI_AllocAslRequest(register __d0 unsigned long reqType, register __a0 struct TagItem *tagList)
#else
	AROS_LH2(APTR, MUI_AllocAslRequest,

/*  SYNOPSIS */
	AROS_LHA(unsigned long, reqType, D0),
	AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 8, MUIMaster)
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

    return AllocAslRequest(reqType,tagList);

    AROS_LIBFUNC_EXIT

} /* MUIA_AllocAslRequest */
