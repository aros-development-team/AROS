/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function DisplayBeep()
    Lang: english
*/
#include "intuition_intern.h"

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
    struct timerequest *MyTimerIO = NULL;
    BOOL VisualBeep = TRUE;
    
    TimerMsgPort = CreateMsgPort();
    MyTimerIO = (struct timerequest *) CreateIORequest (TimerMsgPort, sizeof (struct timerequest));
    OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest *) MyTimerIO, 0);
    
    MyTimerIO->tr_time.tv_secs = 0;
    MyTimerIO->tr_time.tv_micro = 250000;
    MyTimerIO->tr_node.io_Command = TR_ADDREQUEST;
  
    if (VisualBeep)
    {
	if (! screen)
            screen = IntuitionBase->FirstScreen;

	if (screen->RastPort.BitMap->Depth <= 8)
	// visual beep on CLUT-screen
	{
            struct DrawInfo *DInfo = NULL;
            UWORD BGPen;
            ULONG color[3];

	    DInfo = GetScreenDrawInfo (screen);
	    BGPen = DInfo->dri_Pens[BACKGROUNDPEN];

	    GetRGB32 (screen->ViewPort.ColorMap, BGPen, 1, color);

            screen->Flags |= BEEPING;
	    SetRGB32 (&screen->ViewPort, BGPen, color[0] - 0x7FFFFFFF, color[1] - 0x7FFFFFFF, color[2] - 0x7FFFFFFF);
	    DoIO ((struct IORequest *) MyTimerIO);
	    SetRGB32 (&screen->ViewPort, BGPen, color[0], color[1], color[2]);
	    screen->Flags &= !BEEPING;

	    FreeScreenDrawInfo (screen, DInfo);
	}
	else
	// visual beep on hi- and truecolor screens
	{
            struct Window *BeepWindow;
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
            BeepWindow = (struct Window *) OpenWindowTagList (NULL, window_tags);
            if (BeepWindow)
	    {
                DoIO ((struct IORequest *) MyTimerIO);
                CloseWindow (BeepWindow);
	    }
	}   
    }
    
    CloseDevice ((struct IORequest *) MyTimerIO);
    DeleteIORequest ((struct IORequest *) MyTimerIO);
    DeleteMsgPort (TimerMsgPort);

#warning TODO: Produce beep according to prefs
#warning TODO: Handle BEEPING flag correctly
#warning TODO: Make BeepWindow 50% transparent! :-) 
#warning TODO: Use TimerIO (IntuitionBase->TimerIO) instead of self-made MyTimerIO!

    AROS_LIBFUNC_EXIT
} /* DisplayBeep */
