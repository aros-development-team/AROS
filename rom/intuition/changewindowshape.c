/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

#ifdef ChangeLayerShape

struct ChangeWindowShapeActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    struct Region   	    *shape;
    struct Hook     	    *callback;
};

static VOID int_changewindowshape(struct ChangeWindowShapeActionMsg *msg,
                                  struct IntuitionBase *IntuitionBase);

#endif

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH3(struct Region *, ChangeWindowShape,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),
        AROS_LHA(struct Region *, newshape, A1),
        AROS_LHA(struct Hook *, callback, A2),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 143, Intuition)

/*  FUNCTION

    INPUTS
        window - The window to affect.

    RESULT

    NOTES
        This function is also present in MorphOS v50, however
        not implemented and reserved.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef ChangeLayerShape
    struct ChangeWindowShapeActionMsg msg;

    ASSERT_VALID_PTR(window);

    if (IS_GZZWINDOW(window)) return NULL;

    msg.window   = window;
    msg.shape    = newshape;
    msg.callback = callback;
    DoSyncAction((APTR)int_changewindowshape, &msg.msg, IntuitionBase);

    return msg.shape;
#else

    /* shut up the compiler */
    IntuitionBase = IntuitionBase;
    callback = callback;
    newshape = newshape;
    window = window;

    return NULL;
#endif

    AROS_LIBFUNC_EXIT

} /* ChangeWindowShape */


#ifdef ChangeLayerShape
static VOID int_changewindowshape(struct ChangeWindowShapeActionMsg *msg,
                                  struct IntuitionBase *IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Window   *window = msg->window;
    struct Region   *shape = msg->shape;
    struct Hook     *callback = msg->callback;
    struct Screen   *screen = window->WScreen;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;
    
    LOCK_REFRESH(screen);
    msg->shape = ChangeLayerShape(window->WLayer, shape, callback);
    UNLOCK_REFRESH(screen);

    CheckLayers(screen, IntuitionBase);
}
#endif
