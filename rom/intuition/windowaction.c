/*
    Copyright  2003, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <utility/tagitem.h>
#include <intuition/extensions.h>

#include "intuition_intern.h"
#include "inputhandler_support.h"

AROS_LH3(void, WindowAction,
    AROS_LHA(struct Window *, window, A0),
    AROS_LHA(ULONG, action, D0),
    AROS_LHA(struct TagItem *, tags, A1),
    struct IntuitionBase *, IntuitionBase, 157, Intuition
)
{
    AROS_LIBFUNC_INIT
    
#   warning TODO: Write intuition/WindowAction()

    switch (action)
    {
        case WAC_SENDIDCMPCLOSE:
            ih_fire_intuimessage(window, IDCMP_CLOSEWINDOW, 0, 0, IntuitionBase);
            break;

        default:
            aros_print_not_implemented ("WindowAction");
            break;
    }

    AROS_LIBFUNC_EXIT
} /* WindowAction */
