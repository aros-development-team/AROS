/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction listbrowser.gadget - GetClass entry point
*/

#include <intuition/classes.h>
#include "listbrowser_intern.h"

/*****************************************************************************

    NAME */
#include <proto/listbrowser.h>

        AROS_LH0(Class *, LISTBROWSER_GetClass,

/*  LOCATION */
        struct Library *, ListBrowserBase, 5, ListBrowser)

/*  FUNCTION
        Returns a pointer to the listbrowser BOOPSI class.

    RESULT
        Pointer to the class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(ListBrowserBase);

    AROS_LIBFUNC_EXIT
}
