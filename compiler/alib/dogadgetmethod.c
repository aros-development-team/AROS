/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.
*/

#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif
#include "alib_intern.h"
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/alib.h>

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

        IPTR DoGadgetMethod (

/*  SYNOPSIS */
        struct Gadget    * gad,
        struct Window    * win,
        struct Requester * req,
        ULONG              methodId,
        ...)

/*  FUNCTION
        Invokes a BOOPSI method on a object with a GadgetInfo derived from
        the supplied window or requester parameter.

    INPUTS
        gad - The gadget to work on
        win - The window which contains the gadget or the requester with
                the gadgets.
        req - If the gadget is in a requester, you must specify that one,
                too.
        methodId - The message to the gadget follows here.

    RESULT
        The result depends on the contents of the message sent to the
        gadget.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKMETHODS_PRE(methodId)
    retval = DoGadgetMethodA (gad
        , win
        , req
        , AROS_SLOWSTACKMETHODS_ARG(methodId)
    );
    AROS_SLOWSTACKMETHODS_POST
} /* DoGadgetMethod */
