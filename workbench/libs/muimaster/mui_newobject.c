/*
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/intuition.h>

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(Object *, MUI_NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(char *, classname, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 5, MUIMaster)

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

    Class  *cl;

    cl = MUI_GetClass(classname);
    if (cl)
    {
#ifndef __MAXON__
#warning FIXME: I should increase the open count of library (use cl->hook->data)
#endif
	Object *obj = NewObjectA(cl, NULL, tags);

	if (!obj)
	{
            bug("*** Could not create object of %s\n", classname);
        }

	return obj;
    }

    bug("*** Couldn't find %s\n", classname);

    return NULL;

    AROS_LIBFUNC_EXIT
} /* MUIA_NewObjectA */
