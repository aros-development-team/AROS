/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include "muimaster_intern.h"
#include "support_classes.h"

/*****************************************************************************

    NAME */
	AROS_LH1(BOOL, MUI_FreeClass,

/*  SYNOPSIS */
	AROS_LHA(Class *, cl, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 14, MUIMaster)

/*  FUNCTION
	Frees a class returned by MUI_GetClass().

    INPUTS
        cl - The pointer to the class.

    RESULT
        BOOL - Unlike MUI's version of MUI_FreeClass(), Zune's MUI_FreeClass() returns
	       a BOOL value indicating success or failure. This is mostly used for internal
	       purposes and shouldn't be relied upon, rather, you should foget MUI_FreeClass()
	       even exists and use MUI_DeleteCustomClass() & friends instead.

    NOTES
        This function is obsolete, DO NOT use it.

    EXAMPLE

    BUGS

    SEE ALSO
        MUI_GetClass(), MUI_CreateCustomClass(), MUI_DeleteCustomClass()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *, MUIMasterBase)

    /* CLF_INLIST tells us that this class is a builtin class */
    if (cl->cl_Flags & CLF_INLIST)
    {
        ZUNE_RemoveBuiltinClass(cl, MUIMasterBase);

        if (!FreeClass(cl))
        {
            /* If it was a builtin class, readd it to the list since freeing it failed */
            ZUNE_AddBuiltinClass(cl, MUIMasterBase);

	    return FALSE;
	}
	/* If the class could be freed then also close muimaster.library to decrease the
	   reference count.  */
	else CloseLibrary(MUIMasterBase);
    }
    else CloseLibrary((struct Library *)cl->cl_Dispatcher.h_Data);

    return TRUE;

    AROS_LIBFUNC_EXIT

} /* MUIA_FreeClass */
