/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function DisplayBeep()
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/graphics.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, DisplayBeep,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 16, Intuition)

/*  FUNCTION
	The Amiga has no internal speaker, so it flashes the background
	color of the specified screen as a signal. If the argument is
	NULL all screens will be flashed.

    INPUTS
	screen - The Screen that will be flashed.
		If NULL all screens will flash.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Hardware with a speaker should make an audible beep, too.
	Maybe even leave out the flashing on those architectures.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct MsgPort *TimerMsgPort = NULL;
    struct timerequest *BeepTimerIO = NULL;
    BOOL VisualBeep = TRUE;
    BOOL AllScreens = FALSE;

    TimerMsgPort = CreateMsgPort();
    BeepTimerIO = (struct timerequest *) CreateIORequest (TimerMsgPort, sizeof (struct timerequest));
    OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest *) BeepTimerIO, 0);

    BeepTimerIO->tr_time.tv_secs = 0;
    BeepTimerIO->tr_time.tv_micro = 250000;
    BeepTimerIO->tr_node.io_Command = TR_ADDREQUEST;


/* Start Beep */
    if (VisualBeep)
    {
	if (! screen)
	{
	    //LockIBase();
            screen = IntuitionBase->FirstScreen;
            //UnlockIBase();
	    AllScreens = TRUE;
	}

	do
	{
	    if (!(screen->Flags & BEEPING))
	    {
		if (GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) <= 8)
		// visual beep on CLUT-screen
		{
		    /*
		    struct DrawInfo *DInfo = NULL;
		    UWORD BGPen;
		    ULONG color[3];

		    DInfo = GetScreenDrawInfo (screen);
		    BGPen = DInfo->dri_Pens[BACKGROUNDPEN];
		    */

		    Forbid();

		    GetRGB32 (screen->ViewPort.ColorMap, 0, 1, GetPrivScreen(screen)->DisplayBeepColor0);
		    screen->SaveColor0 = GetRGB4 (screen->ViewPort.ColorMap, 0);

		    SetRGB32 (&screen->ViewPort, 0,
			      GetPrivScreen(screen)->DisplayBeepColor0[0] - 0x7FFFFFFF,
			      GetPrivScreen(screen)->DisplayBeepColor0[1] - 0x7FFFFFFF,
			      GetPrivScreen(screen)->DisplayBeepColor0[2] - 0x7FFFFFFF
			     );
		    screen->Flags |= BEEPING;
		    Permit();

		    // FreeScreenDrawInfo (screen, DInfo);
		}
		else
		// visual beep on hi- and truecolor screens
		{
		    // struct Window *BeepWindow;
		    struct TagItem window_tags[] = {
			    {WA_Left, 0},
			    {WA_Top, 0},
			    {WA_Width, -1},
			    {WA_Height, -1},
			    {WA_Flags, WFLG_SIMPLE_REFRESH | WFLG_BORDERLESS},
			    {WA_CustomScreen, (IPTR) screen},
			    {WA_Priority, 50},  // Place in front of all other windows!
			    {TAG_DONE, 0}
		    };

		    Forbid();
		    GetPrivScreen(screen)->DisplayBeepWindow = (struct Window *) OpenWindowTagList (NULL, window_tags);
		    screen->Flags |= BEEPING;
		    Permit();
		}

		//LockIBase();
		if (AllScreens) screen = screen->NextScreen;
		//UnlockIBase();
	    }
	} while (AllScreens && screen);
    }

/* Wait */
    DoIO ((struct IORequest *) BeepTimerIO);

/* Stop Beep */
    if (VisualBeep)
    {
	if (AllScreens)
	{
            //LockIBase();
	    screen = IntuitionBase->FirstScreen;
            //UnlockIBase();
	}

	do
	{
            if (screen->Flags & BEEPING)
	    {
		if (GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) <= 8)
		// visual beep on CLUT-screen
		{
		    Forbid();
		    // SetRGB4 (&screen->ViewPort, 0, screen->SaveColor0 & 0x000F, (screen->SaveColor0 & 0x00F0) >> 4, (screen->SaveColor0 & 0x0F00) >> 8);
		    SetRGB32 (&screen->ViewPort, 0,
			      GetPrivScreen(screen)->DisplayBeepColor0[0],
			      GetPrivScreen(screen)->DisplayBeepColor0[1],
			      GetPrivScreen(screen)->DisplayBeepColor0[2]
			     );
		    screen->Flags &= ~BEEPING;
		    Permit();
		}
		else
		// visual beep on hi- and truecolor screens
		{
		    if (GetPrivScreen(screen)->DisplayBeepWindow)
		    {
			Forbid();
			CloseWindow (GetPrivScreen(screen)->DisplayBeepWindow);
			GetPrivScreen(screen)->DisplayBeepWindow = NULL;
			screen->Flags &= ~BEEPING;
			Permit();
		    }
		}
            }

            //LockIBase();
	    if (AllScreens) screen = screen->NextScreen;
	    //UnlockIBase();

	} while (AllScreens && screen);
    }


    CloseDevice ((struct IORequest *) BeepTimerIO);
    DeleteIORequest ((struct IORequest *) BeepTimerIO);
    DeleteMsgPort (TimerMsgPort);

#warning TODO: Make this "multitasking proof"!
#warning TODO: Produce beep according to prefs
#warning TODO: Check if BEEPING flag is handled correctly
#warning TODO: Make BeepWindow 50% transparent! :-)
#warning TODO: Use TimerIO (IntuitionBase->TimerIO) instead of self-made BeepTimerIO?

    AROS_LIBFUNC_EXIT
} /* DisplayBeep */
