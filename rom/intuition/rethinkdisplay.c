/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <intuition/pointerclass.h>
#include <proto/graphics.h>

#include "intuition_intern.h"
#include "inputhandler.h"
#include "monitorclass_private.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

    AROS_LH0(LONG, RethinkDisplay,

/*  SYNOPSIS */

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 65, Intuition)

/*  FUNCTION
    Check and update, ie. redisplay the whole Intuition display.

    INPUTS
    None.

    RESULT
    Zero for success, non-zero for failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    RemakeDisplay(), MakeScreen(), graphics.library/MakeVPort(),
    graphics.library/MrgCop(), graphics.library/LoadView()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RethinkDisplayActionMsg msg;

    msg.lock = TRUE;
    DoSyncAction((APTR)int_RethinkDisplay, &msg.msg, IntuitionBase);

    return msg.failure;

    AROS_LIBFUNC_EXIT
} /* RethinkDisplay */


void int_RethinkDisplay(struct RethinkDisplayActionMsg *msg,struct IntuitionBase *IntuitionBase)
{
    struct GfxBase   *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Screen    *screen;
    struct ViewPort  *viewport;
    struct ViewPort **viewportptr;
    UWORD   	      modes;
    LONG    	      failure = 0;
    ULONG   	      ilock;

    if (msg->lock)
        ilock = LockIBase(0);

    DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay:\n"));

    screen = IntuitionBase->FirstScreen;

    DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: screen 0x%lx\n",screen));

    if (screen == NULL)
    {
        DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: LoadView(NULL)\n"));
        IntuitionBase->ViewLord.ViewPort = NULL;
        LoadView(NULL);
    }
    else
    {
        /* Find visible screens */
        DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: Find visible screens\n"));

        screen->ViewPort.Modes |= SPRITES;

        screen->ViewPort.Modes &= ~VP_HIDE;

	/* Not needed. Perhaps... Pavel Fedin <pavel_fedin@mail.ru>
	if (!(GetPrivScreen(screen)->SpecialFlags & SF_Compose)) {
            while ((screen = screen->NextScreen))
                screen->ViewPort.Modes |= VP_HIDE;
        }
	*/
        /* Build the list of viewports in the view */
	DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: Building viewports list\n"));
        viewportptr = &IntuitionBase->ViewLord.ViewPort;
        for (screen = IntuitionBase->FirstScreen; screen; screen = screen->NextScreen)
        {
            if ((screen->ViewPort.Modes & VP_HIDE) == 0)
            {
		DEBUG_RETHINKDISPLAY(bug("RethinkDisplay: Adding ViewPort 0x%p for screen 0x%p\n", &screen->ViewPort, screen));
                *viewportptr = &screen->ViewPort;
                viewportptr = &screen->ViewPort.Next;
            }
        }
        *viewportptr = NULL;

        /* Find view mode */
	DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: Find view mode\n"));
        modes = (IntuitionBase->ViewLord.Modes & ~LACE) | SPRITES;
        for (viewport = IntuitionBase->ViewLord.ViewPort; viewport; viewport = viewport->Next) {
            modes |= viewport->Modes & LACE;
	}

        /* Reinitialize the view */
#ifdef __MORPHOS__
	/* Under AROS this will cause unneeded screen flicker and even significant slowdown */
        FreeSprite(GetPrivIBase(IntuitionBase)->SpriteNum);
        GetPrivIBase(IntuitionBase)->SpriteNum = -1;
        DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: LoadView(NULL)\n"));

        LoadView(NULL);

        if (IntuitionBase->ViewLord.LOFCprList)
            FreeCprList(IntuitionBase->ViewLord.LOFCprList);

        if (IntuitionBase->ViewLord.SHFCprList)
            FreeCprList(IntuitionBase->ViewLord.SHFCprList);

        IntuitionBase->ViewLord.LOFCprList = NULL;
        IntuitionBase->ViewLord.SHFCprList = NULL;
#endif
        IntuitionBase->ViewLord.DxOffset = 0; /***/
        IntuitionBase->ViewLord.DyOffset = 0; /***/
        IntuitionBase->ViewLord.Modes = modes;
        screen = IntuitionBase->FirstScreen;
#ifdef __MORPHOS__
        GetPrivIBase(IntuitionBase)->ViewLordExtra->Monitor = GetPrivScreen(screen)->Monitor;
#endif
        /* Rebuild copper lists for screens needing a mode change */

	DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: Making viewports\n"));
        for (viewport = IntuitionBase->ViewLord.ViewPort; viewport; viewport = viewport->Next)
        {
            if ((viewport->Modes ^ modes) & LACE)
            {
                LONG error;
		
                viewport->Modes = (viewport->Modes & ~LACE) | (modes & LACE);
                error = MakeVPort(&IntuitionBase->ViewLord, viewport);
		
                if (error)
                    failure = error;
            }
        }

        if (!failure)
        {
            failure = MrgCop(&IntuitionBase->ViewLord);

            if (!failure)
            {
                struct MinNode *m;

                DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: LoadView ViewLord 0x%lx\n",&IntuitionBase->ViewLord));
                LoadView(&IntuitionBase->ViewLord);

                /*
                 * Set gamma correction for all displays.
                 * We do it after LoadView() because some video chipsets reuse the
                 * same registers for both LUT palette and gamma table. Consequently,
                 * we may need to reload gamma table after the mode has been changed.
                 */
                ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);

                for (m = GetPrivIBase(IntuitionBase)->MonitorList.mlh_Head;
                     m->mln_Succ; m = m->mln_Succ)
                {
                    struct Screen *scr = FindFirstScreen((Object *)m, IntuitionBase);

                    DoMethod((Object *)m, MM_SetScreenGamma, scr);
	        }

                ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

                DEBUG_INIT(dprintf("RethinkDisplay: ActiveScreen %p Pointer %p Sprite %p\n",
                            IntuitionBase->ActiveScreen,
                            GetPrivScreen(IntuitionBase->ActiveScreen)->Pointer,
                            GetPrivScreen(IntuitionBase->ActiveScreen)->Pointer->sprite));
#ifdef __MORPHOS__
                if (GetPrivIBase(IntuitionBase)->SpriteNum == -1 &&
                        IntuitionBase->FirstScreen &&
                        GetPrivScreen(IntuitionBase->FirstScreen)->Pointer &&
                        GetPrivScreen(IntuitionBase->FirstScreen)->Pointer->sprite)
                {
                    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
                    WORD    	    xpos,ypos;

                    xpos = iihd->LastMouseX; ypos = iihd->LastMouseY;
                    GetPrivIBase(IntuitionBase)->SpriteNum = GetExtSpriteA(GetPrivScreen(IntuitionBase->FirstScreen)->Pointer->sprite, NULL);

                    DEBUG_INIT(dprintf("RethinkDisplay: Sprite num %ld\n", GetPrivIBase(IntuitionBase)->SpriteNum));

                    if (xpos >= IntuitionBase->FirstScreen->Width) xpos = IntuitionBase->FirstScreen->Width - 1;
                    if (ypos >= IntuitionBase->FirstScreen->Height) ypos = IntuitionBase->FirstScreen->Height - 1;

                    MySetPointerPos(IntuitionBase, xpos, ypos);
                    IntuitionBase->FirstScreen->MouseX = xpos;
                    IntuitionBase->FirstScreen->MouseY = ypos;
                }
#endif
            }
        }
    }

    if (!failure)
    {
        /* validate screen positions, scrolling limits are not necessarily same anymore. */
        for (screen = IntuitionBase->FirstScreen; screen; screen = screen->NextScreen)
        {
            if ((screen->ViewPort.Modes & VP_HIDE) == 0)
            {
                ScreenPosition( screen, SPOS_RELATIVE, 0,0,0,0  );
            }
        }
        /* Ensure that empty displays get normal pointer */
        ResetPointer(IntuitionBase);
    }

    if (msg->lock)
        UnlockIBase(ilock);

    DEBUG_RETHINKDISPLAY(dprintf("RethinkDisplay: failure 0x%lx\n",failure));

    msg->failure = failure;
}
