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
__asm BOOL MUI_AslRequest(register __a0 APTR requester, register __a1 struct TagItem *tagList)
#else
	AROS_LH2(BOOL, MUI_AslRequest,

/*  SYNOPSIS */
	AROS_LHA(APTR, requester, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 9, MUIMaster)
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

    return AslRequest(requester,tagList);

    AROS_LIBFUNC_EXIT

} /* MUIA_AslRequest */
