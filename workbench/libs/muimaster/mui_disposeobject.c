/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>
#include <proto/intuition.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH1(VOID, MUI_DisposeObject,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 6, MUIMaster)

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

    Class *cl = OCLASS(obj);

    DisposeObject(obj);

    MUI_FreeClass(cl);

    AROS_LIBFUNC_EXIT

} /* MUIA_DisposeObject */
