/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - RA_HandleInput() implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <classes/window.h>

#include "reaction_windowmethods.h"
#include "reaction_intern.h"

/*****************************************************************************

    NAME */
#include <proto/reaction.h>

        AROS_LH2(ULONG, RA_HandleInput,

/*  SYNOPSIS */
        AROS_LHA(Object *, windowobj, A0),
        AROS_LHA(WORD *, code, A1),

/*  LOCATION */
        struct ReactionBase *, ReactionBase, 7, Reaction)

/*  FUNCTION
        Process input events for a window.class object. Call this repeatedly
        in your event loop until it returns WMHI_LASTMSG.

    INPUTS
        windowobj - Pointer to a window.class object.
        code      - Pointer to a WORD to receive additional event data.

    RESULT
        A message class identifier (WMHI_*). Returns WMHI_LASTMSG when
        there are no more messages to process.

    NOTES
        Typical usage:
            while ((result = RA_HandleInput(windowobj, &code)) != WMHI_LASTMSG)
            {
                switch (result & WMHI_CLASSMASK)
                {
                    case WMHI_CLOSEWINDOW:
                        ...
                    case WMHI_GADGETUP:
                        switch (result & WMHI_GADGETMASK)
                        {
                            ...
                        }
                }
            }

    SEE ALSO
        RA_OpenWindow(), RA_CloseWindow()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG result = WMHI_LASTMSG;

    if (windowobj)
    {
        result = DoMethod(windowobj, WM_HANDLEINPUT, (IPTR)code);
    }

    return result;

    AROS_LIBFUNC_EXIT
}
