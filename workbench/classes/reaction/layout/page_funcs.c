/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction layout.gadget - Page class functions
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/layout.h>

#include "layout_intern.h"

/* Static page class pointer */
static Class *pageclass = NULL;

/*****************************************************************************

    NAME */
#include <proto/layout.h>

        AROS_LH0(Class *, PAGE_GetClass,

/*  LOCATION */
        struct Library *, LayoutBase, 10, Layout)

/*  FUNCTION
        Returns a pointer to the Page gadget class.

    RESULT
        Pointer to the Page class, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Return the page class - in a full implementation this would
       be a proper registered BOOPSI class */
    return pageclass;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH5(ULONG, SetPageGadgetAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Gadget *, gadget, A0),
        AROS_LHA(Object *, object, A1),
        AROS_LHA(struct Window *, window, A2),
        AROS_LHA(struct Requester *, requester, A3),
        AROS_LHA(struct TagItem *, tags, A4),

/*  LOCATION */
        struct Library *, LayoutBase, 11, Layout)

/*  FUNCTION
        Sets attributes on a gadget that is a child of a page layout.

    INPUTS
        gadget    - The gadget to modify.
        object    - The page layout object.
        window    - The window containing the gadget.
        requester - The requester (may be NULL).
        tags      - Tag list of attributes to set.

    RESULT
        Return value from SetGadgetAttrsA.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (gadget && window)
    {
        return SetGadgetAttrsA(gadget, window, requester, tags);
    }
    return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(void, RefreshPageGadget,

/*  SYNOPSIS */
        AROS_LHA(struct Gadget *, gadget, A0),
        AROS_LHA(Object *, object, A1),
        AROS_LHA(struct Window *, window, A2),
        AROS_LHA(struct Requester *, requester, A3),

/*  LOCATION */
        struct Library *, LayoutBase, 12, Layout)

/*  FUNCTION
        Refreshes a gadget within a page layout.

    INPUTS
        gadget    - The gadget to refresh.
        object    - The page layout object.
        window    - The window containing the gadget.
        requester - The requester (may be NULL).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (gadget && window)
    {
        RefreshGList(gadget, window, requester, 1);
    }

    AROS_LIBFUNC_EXIT
}
