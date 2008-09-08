/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"

/* #define MYDEBUG 1 */
#include "debug.h"

/*****************************************************************************

    NAME */
	AROS_LH2(Object *, MUI_NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(ClassID, classid,     A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 5, MUIMaster)

/*  FUNCTION
	Create object from MUI class.

    INPUTS
	classid - case sensitive name/ID string of a MUI class.
	taglist - attribute/value pairs for the new object.

    RESULT
	Pointer to object. NULL means failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MUI_DisposeObject(), intuition.library/SetAttrs()
	intuition.library/GetAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    Class  *cl;

    cl = MUI_GetClass(classid);
    if (cl)
    {
	Object *obj = NewObjectA(cl, NULL, tags);

	if (obj) return obj;

        bug("*** Could not create object of %s\n", classid);
	MUI_FreeClass(cl);
    }

    bug("*** Couldn't find %s\n", classid);

    return NULL;

    AROS_LIBFUNC_EXIT
} /* MUIA_NewObjectA */
