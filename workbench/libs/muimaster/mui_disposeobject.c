/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/intuition.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm VOID MUI_DisposeObject(register __a0 Object *obj)
#else
	AROS_LH1(VOID, MUI_DisposeObject,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 6, MUIMaster)
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

	MUI will call DisposeObject(), then call CloseLibrary() on
	OCLASS(obj)->h_Data if cl_ID!=NULL && h_Data!=NULL.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

#ifndef __MAXON__
#warning FIXME: I should decrease the open count of library (use cl->hook->data)
#endif
    DisposeObject(obj);

    AROS_LIBFUNC_EXIT

} /* MUIA_DisposeObject */
