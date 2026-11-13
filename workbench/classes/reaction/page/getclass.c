/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: page.gadget - GetClass entry point.

    Note: PAGE_GetClass() is exported from layout.gadget at LVO 10 for
    ABI compatibility with ClassAct/ReAction; layout.gadget opens this
    library on demand and resolves the class via FindClass(). To avoid
    a symbol collision in <proto/layout.h>, our own accessor is named
    Page_GetClass.
*/

#include <intuition/classes.h>
#include "page_intern.h"

/*****************************************************************************

    NAME */
#include <proto/page.h>

        AROS_LH0(Class *, Page_GetClass,

/*  LOCATION */
        struct Library *, PageBase, 5, Page)

/*  FUNCTION
        Returns a pointer to the page BOOPSI class.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (Class *)GM_CLASSPTR_FIELD(PageBase);

    AROS_LIBFUNC_EXIT
}
