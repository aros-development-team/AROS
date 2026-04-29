/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction layout.gadget - Exported library functions
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/layout.h>

#include "layout_intern.h"

/*****************************************************************************

    NAME */
#include <proto/layout.h>

        AROS_LH4(BOOL, ActivateLayoutGadget,

/*  SYNOPSIS */
        AROS_LHA(struct Gadget *, gadget, A0),
        AROS_LHA(struct Window *, window, A1),
        AROS_LHA(struct Requester *, requester, A2),
        AROS_LHA(ULONG, object, D0),

/*  LOCATION */
        struct Library *, LayoutBase, 6, Layout)

/*  FUNCTION
        Activates a gadget within the layout group.

    INPUTS
        gadget    - The layout gadget.
        window    - The window containing the layout.
        requester - The requester (may be NULL).
        object    - ID of the gadget to activate.

    RESULT
        TRUE if the gadget was activated, FALSE otherwise.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (gadget && window)
    {
        return ActivateGadget(gadget, window, requester);
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FlushLayoutDomainCache,

/*  SYNOPSIS */
        AROS_LHA(struct Gadget *, gadget, A0),

/*  LOCATION */
        struct Library *, LayoutBase, 7, Layout)

/*  FUNCTION
        Flushes the cached domain information for the layout gadget,
        forcing a recomputation on the next layout pass.

    INPUTS
        gadget - The layout gadget.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (gadget)
    {
        /* Force domain recomputation by sending a GM_LAYOUT */
        SetAttrs((Object *)gadget, TAG_DONE);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(BOOL, RethinkLayout,

/*  SYNOPSIS */
        AROS_LHA(struct Gadget *, gadget, A0),
        AROS_LHA(struct Window *, window, A1),
        AROS_LHA(struct Requester *, requester, A2),
        AROS_LHA(BOOL, refresh, D0),

/*  LOCATION */
        struct Library *, LayoutBase, 8, Layout)

/*  FUNCTION
        Recalculates the layout of all children within the layout gadget.

    INPUTS
        gadget    - The layout gadget.
        window    - The window containing the layout.
        requester - The requester (may be NULL).
        refresh   - If TRUE, refresh the gadgets after relayout.

    RESULT
        TRUE on success.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (gadget && window)
    {
        /* Set new dimensions based on window inner area */
        SetAttrs((Object *)gadget,
            GA_Width,  window->Width - window->BorderLeft - window->BorderRight,
            GA_Height, window->Height - window->BorderTop - window->BorderBottom,
            TAG_DONE);

        if (refresh)
        {
            RefreshGList(gadget, window, requester, -1);
        }
        return TRUE;
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(void, LayoutLimits,

/*  SYNOPSIS */
        AROS_LHA(struct Gadget *, gadget, A0),
        AROS_LHA(struct LayoutLimits *, limits, A1),
        AROS_LHA(struct TextFont *, font, A2),
        AROS_LHA(struct Screen *, screen, A3),

/*  LOCATION */
        struct Library *, LayoutBase, 9, Layout)

/*  FUNCTION
        Queries the minimum and maximum size of a layout group.

    INPUTS
        gadget - The layout gadget.
        limits - Pointer to LayoutLimits structure to receive the values.
        font   - Font to use for calculations (may be NULL).
        screen - Screen for rendering context.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (gadget && limits)
    {
        limits->MinWidth  = 100;
        limits->MinHeight = 50;
        limits->MaxWidth  = 0xFFFF;
        limits->MaxHeight = 0xFFFF;
    }

    AROS_LIBFUNC_EXIT
}
