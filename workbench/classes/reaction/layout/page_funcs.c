/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction layout.gadget - Page-related library functions.

    The actual page.gadget BOOPSI class lives in its own library in
    workbench/classes/reaction/page/. layout.gadget exposes
    PAGE_GetClass() at LVO 10 for ABI compatibility with ClassAct/
    ReAction; it just opens page.gadget and forwards to its class
    accessor. SetPageGadgetAttrsA / RefreshPageGadget are convenience
    wrappers around SetGadgetAttrsA / RefreshGList.
*/
#define DEBUG 1

#include <proto/exec.h>
#include <proto/intuition.h>

#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/layout.h>

#include "layout_intern.h"

/*****************************************************************************/

#include <proto/layout.h>

AROS_LH0(Class *, PAGE_GetClass,
        struct Library *, LayoutBase, 10, Layout)
{
    AROS_LIBFUNC_INIT

    struct LayoutBase_intern *base = (struct LayoutBase_intern *)LayoutBase;

    if (!base->rc_PageBase)
        base->rc_PageBase = OpenLibrary("page.gadget", 0);

    if (base->rc_PageBase)
        return FindClass((CONST_STRPTR)"page.gadget");

    D(bug("[Layout] PAGE_GetClass: page.gadget not available\n"));
    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************/

AROS_LH5(ULONG, SetPageGadgetAttrsA,
        AROS_LHA(struct Gadget *, gadget, A0),
        AROS_LHA(Object *, object, A1),
        AROS_LHA(struct Window *, window, A2),
        AROS_LHA(struct Requester *, requester, A3),
        AROS_LHA(struct TagItem *, tags, A4),
        struct Library *, LayoutBase, 11, Layout)
{
    AROS_LIBFUNC_INIT
    if (gadget && window)
        return SetGadgetAttrsA(gadget, window, requester, tags);
    return 0;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************/

AROS_LH4(void, RefreshPageGadget,
        AROS_LHA(struct Gadget *, gadget, A0),
        AROS_LHA(Object *, object, A1),
        AROS_LHA(struct Window *, window, A2),
        AROS_LHA(struct Requester *, requester, A3),
        struct Library *, LayoutBase, 12, Layout)
{
    AROS_LIBFUNC_INIT
    if (gadget && window)
        RefreshGList(gadget, window, requester, 1);
    AROS_LIBFUNC_EXIT
}
