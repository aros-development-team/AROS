/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Change order of screens.
*/

#include <proto/graphics.h>

#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_actions.h"

struct ScreenDepthActionMsg
{
    struct IntuiActionMsg    msg;
    struct Screen   	    *screen;
    ULONG   	    	     flags;
};

static VOID int_screendepth(struct ScreenDepthActionMsg *msg,
                            struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH3(void, ScreenDepth,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen *, screen, A0),
         AROS_LHA(ULONG          , flags, D0),
         AROS_LHA(APTR           , reserved, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 131, Intuition)

/*  FUNCTION
    Move the specified screen to the front or back, based on passed flag.
    If the screen is in a group, the screen will change its position in
    the group only. If the screen is the parent of a group, the whole
    group will be moved.

    INPUTS
    screen - Move this screen.
    flags - SDEPTH_TOFRONT or SDEPTH_TOBACK for bringing the screen to
        front or back.
        If the screen is a child of another screen you may specify
        SDEPTH_INFAMILY to move the screen within the family. If
        not specified the whole family will move.
    reserved - For future use. MUST be NULL by now.

    RESULT
    None.

    NOTES
    Only the owner of the screen should use SDEPTH_INFAMILY.
    Intentionally commodities should not change the internal arrangement
    of screen families.

    EXAMPLE

    BUGS

    SEE ALSO
    ScreenToBack(), ScreenToFront()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct ScreenDepthActionMsg msg;

    if (reserved != NULL) return;
    SANITY_CHECK(screen)

    msg.screen = screen;
    msg.flags  = flags;
    DoASyncAction((APTR)int_screendepth, &msg.msg, sizeof(msg), IntuitionBase);

    AROS_LIBFUNC_EXIT

} /* ScreenDepth */

/*****************************************************************************************/

static VOID int_screendepth(struct ScreenDepthActionMsg *msg,
                            struct IntuitionBase *IntuitionBase)
{
    struct Screen   *screen = msg->screen;
    ULONG   	     flags = msg->flags;
    ULONG            ilock = LockIBase(0); /* before access to FirstScreen */
    struct Screen   *family = NULL,
                    *current = IntuitionBase->FirstScreen,
#ifndef __MORPHOS__
                    *oldfront = current,
#endif
                    *previous = NULL,
                    *prefamily = NULL;
    struct Window   *win;

    /* Find the screen in the list and check for family */
    while ( current && current!=screen )
    {
        if ( flags & SDEPTH_INFAMILY )
        {
            /* Check if this is the first child in a family */
            if ( !family && (GetPrivScreen(current)->SpecialFlags & SF_IsChild) )
            {
                family = current;
                prefamily = previous;
            }
            /* Check if this screen is a child so next one belongs to current family */
            if ( family && !(GetPrivScreen(current)->SpecialFlags & SF_IsChild) )
            {
                family = NULL;
                prefamily = NULL;
            }
        }
        previous = current;
        current = current->NextScreen;
    }

    if ( current )
    {
        if ( ! (flags & SDEPTH_TOBACK) ) /* SDEPTH_TOFRONT is #defined as 0 */
        {
            if ( previous ) /* I'm not the very first screen */
            {
                if ( flags & SDEPTH_INFAMILY )
                {
                    if ( GetPrivScreen(current)->SpecialFlags & SF_IsChild )
                    { /* Move me in the front of my family */
                        if ( family ) /* I'm not the first one in my family */
                        {
                            previous->NextScreen = current->NextScreen;
                            current->NextScreen = family;
                            if ( prefamily )
                            {
                                prefamily->NextScreen = current;
                            }
                            else
                            {
                                IntuitionBase->FirstScreen = current;
                            }
                        }
                    }
                    else if ( GetPrivScreen(current)->SpecialFlags & SF_IsParent )
                    { /* Move whole family */
                        if ( prefamily ) /* We are not the first family */
                        {
                            prefamily->NextScreen = current->NextScreen;
                            current->NextScreen = IntuitionBase->FirstScreen;
                            IntuitionBase->FirstScreen = family;
                        }
                    }
                    else
                    { /* I have no family */
                        previous->NextScreen = current->NextScreen;
                        current->NextScreen = IntuitionBase->FirstScreen;
                        IntuitionBase->FirstScreen = current;
                    }

                } /* SDEPTH_INFAMILY */
                else
                {
                    if ( GetPrivScreen(current)->SpecialFlags & (SF_IsChild|SF_IsParent) )
                    { /* Move my whole family */
                        if ( !family )
                        {
                            prefamily = previous;
                            family = current;
                        }
                        if ( prefamily )
                        { /* We are not the first family */
                            while ( !(GetPrivScreen(current)->SpecialFlags & SF_IsParent) )
                            {
                                current = current->NextScreen;
                            }
                            prefamily->NextScreen = current->NextScreen;
                            current->NextScreen = IntuitionBase->FirstScreen;
                            IntuitionBase->FirstScreen = family;
                        }
                    }
                    else
                    { /* I have no family */
                        previous->NextScreen = current->NextScreen;
                        current->NextScreen = IntuitionBase->FirstScreen;
                        IntuitionBase->FirstScreen = current;
                    }

                } /* ! SDEPTH_INFAMILY */

            } /* if (previous) */

        } /* if SDEPTH_TO_FRONT */

        else if ( flags & SDEPTH_TOBACK )
        {
            if ( flags & SDEPTH_INFAMILY )
            {
                if ( GetPrivScreen(current)->SpecialFlags & SF_IsChild )
                {
                    /* Go to last screen of this family */
                    while ( !GetPrivScreen(current->NextScreen)->SpecialFlags & SF_IsParent )
                    {
                        current = current->NextScreen;
                    }
                    if ( current != screen ) /* I'm not the last screen of my family */
                    {
                        if ( previous )
                        {
                            previous->NextScreen = screen->NextScreen;
                        }
                        else
                        {
                            IntuitionBase->FirstScreen = screen->NextScreen;
                        }
                        screen->NextScreen = current->NextScreen;
                        current->NextScreen = screen;
                    }
                }
                else if ( GetPrivScreen(current)->SpecialFlags & SF_IsParent )
                {
                    if ( current->NextScreen ) /* I'm not the last screen */
                    {
                        while ( current->NextScreen )
                        {
                            current = current->NextScreen;
                        }
                        if ( prefamily )
                        {
                            prefamily->NextScreen = screen->NextScreen;
                        }
                        else
                        {
                            IntuitionBase->FirstScreen = screen->NextScreen;
                        }
                        if ( family )
                        {
                            current->NextScreen = family;
                        }
                        else
                        {
                            current->NextScreen = screen;
                        }
                        screen->NextScreen = NULL;
                    }
                }
                else
                {
                    if ( current->NextScreen ) /* I'm not the last screen */
                    {
                        while ( current->NextScreen )
                        {
                            current = current->NextScreen;
                        }
                        if ( previous )
                        {
                            previous->NextScreen = screen->NextScreen;
                        }
                        else
                        {
                            IntuitionBase->FirstScreen = screen->NextScreen;
                        }
                        current->NextScreen = screen;
                        screen->NextScreen = NULL;
                    }
                }

            } /* SDEPTH_INFAMILY */
            else
            {
                struct Screen *last;

                if ( GetPrivScreen(current)->SpecialFlags & (SF_IsChild|SF_IsParent) )
                {
                    if ( !family )
                    {
                        family = current;
                        prefamily = previous;
                    }
                    /* Go to last screen of this family */
                    while ( !GetPrivScreen(current)->SpecialFlags & SF_IsParent )
                    {
                        current = current->NextScreen;
                    }
                    if ( current->NextScreen ) /* We are not the last family */
                    {
                        last = current->NextScreen;
                        while ( last->NextScreen )
                        {
                            last = last->NextScreen;
                        }
                        if ( prefamily )
                        {
                            prefamily->NextScreen = current->NextScreen;
                        }
                        else
                        {
                            IntuitionBase->FirstScreen = current->NextScreen;
                        }
                        last->NextScreen = family;
                        current->NextScreen = NULL;
                    }

                } /* if ( GetPrivScreen(current)->SpecialFlags & (SF_IsChild|SF_IsParent) ) */
                else
                {
                    if ( current->NextScreen ) /* I'm not the last screen */
                    {
                        while ( current->NextScreen )
                        {
                            current = current->NextScreen;
                        }
                        if ( previous )
                        {
                            previous->NextScreen = screen->NextScreen;
                        }
                        else
                        {
                            IntuitionBase->FirstScreen = screen->NextScreen;
                        }
                        current->NextScreen = screen;
                        screen->NextScreen = NULL;
                    }

                } /* current not SF_isChild | SF_IsParent */

            } /* ! SDEPTH_INFAMILY */

        } /* if SDEPTH_TO_BACK */

    } /* if (current) */

#ifdef __MORPHOS__
    IntuitionBase->ActiveScreen = IntuitionBase->FirstScreen;
    RethinkDisplay();
#else
    if (IntuitionBase->FirstScreen != oldfront)
    {
        SetFrontBitMap(IntuitionBase->FirstScreen->RastPort.BitMap, TRUE);
        IntuitionBase->ActiveScreen = IntuitionBase->FirstScreen;
    }
#endif

    win = NULL;
#if 0 /* FIXME: backport, disabled */
    if (IntuitionBase->FirstScreen && GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_SCREENACTIVATION)
    {
        struct Window *scanw = 0;

        for (scanw = IntuitionBase->FirstScreen->FirstWindow; scanw ; scanw = scanw->NextWindow)
        {
            if (win)
            {
                if ((IW(scanw)->activationtime.tv_secs > IW(win)->activationtime.tv_secs) ||
                    ((IW(scanw)->activationtime.tv_secs == IW(win)->activationtime.tv_secs) && (IW(scanw)->activationtime.tv_micro > IW(win)->activationtime.tv_micro)))
                {
                    win = scanw;
                }
            }

            if (!win) win = scanw;
        }

        if (!win) win = IntuitionBase->FirstScreen->FirstWindow;
        if (IntuitionBase->ActiveWindow && IntuitionBase->ActiveWindow->WScreen == IntuitionBase->FirstScreen) win = NULL;
    }
#endif

    /* now set the default pub screen */
    /* if the screen is not a public one we just ignore this */

    if (IntuitionBase->FirstScreen && GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_DEFPUBSCREEN)
    {
        if (GetPrivScreen(IntuitionBase->FirstScreen)->pubScrNode && (IntuitionBase->FirstScreen->Flags & (PUBLICSCREEN | WBENCHSCREEN)))
        {
            GetPrivIBase(IntuitionBase)->DefaultPubScreen = IntuitionBase->FirstScreen;
        }
    }

    UnlockIBase(ilock);

    if (win)
    {
        ActivateWindow(win);
    }
}
