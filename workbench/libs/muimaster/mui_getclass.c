/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"
#include "support_classes.h"

/*#define MYDEBUG 1*/
#include "debug.h"

/*****************************************************************************

    NAME */
	AROS_LH1(struct IClass *, MUI_GetClass,

/*  SYNOPSIS */
	AROS_LHA(ClassID, classid, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 13, MUIMaster)

/*  FUNCTION
        Get a pointer to a MUI Class.

	The main use for this function is to retrieve the pointer to a MUI class
	for use by intuition.library/MakeClass() as superclass pointer. However,
	this function is obsolete since MUI V8, so DO NOT USE IT, use
	MUI_CreateCustomClass() instead.

    INPUTS
	classid - the ID of the class whose pointer is to be retrieved.

    RESULT
        The class pointer is returned. DO NOT use it for any other reason that
	as an argument of intuition.library/MakeClass(). DO NOT assume anything
	about its content.

    NOTES
        Once the pointer is not needed anymore, do not forget to call MUI_FreeClass().

        This function is OBSOLETE, don't use it, use MUI_CreateCustomClass instead.

    EXAMPLE

    BUGS

    SEE ALSO
        MUI_FreeClass(), MUI_CreateCustomClass(), MUI_DeleteCustomClass()


    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    Class *cl;

    if (!classid)
	return NULL;

    cl = ZUNE_GetBuiltinClass(classid, MUIMasterBase);

    if (!cl)
        cl = ZUNE_GetExternalClass(classid, MUIMasterBase);

    return cl;

    AROS_LIBFUNC_EXIT

} /* MUIA_GetClass */

