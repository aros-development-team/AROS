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

    ULONG color[3];
    UWORD BGPen;
    struct DrawInfo *DInfo = NULL;
    struct MsgPort *TimerMsgPort = NULL;
    struct timerequest *MyTimerIO = NULL;
   
    TimerMsgPort = CreateMsgPort();
    MyTimerIO = (struct timerequest *) CreateIORequest (TimerMsgPort, sizeof (struct timerequest));
    OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest *) MyTimerIO, 0);
    
    MyTimerIO->tr_time.tv_secs = 0;
    MyTimerIO->tr_time.tv_micro = 200000;
    MyTimerIO->tr_node.io_Command = TR_ADDREQUEST;
  
    if (! screen)
        screen = IntuitionBase->FirstScreen;

    DInfo = GetScreenDrawInfo (screen);
    BGPen = DInfo->dri_Pens[BACKGROUNDPEN];
	
    GetRGB32 (screen->ViewPort.ColorMap, BGPen, 1, color);

    SetRGB32 (&screen->ViewPort, BGPen, color[0] - 0x7FFFFFFF, color[1] - 0x7FFFFFFF, color[2] - 0x7FFFFFFF);
    DoIO ((struct IORequest *) MyTimerIO);
    SetRGB32 (&screen->ViewPort, BGPen, color[0], color[1], color[2]);

    FreeScreenDrawInfo (screen, DInfo);

    CloseDevice ((struct IORequest *) MyTimerIO);
    DeleteIORequest ((struct IORequest *) MyTimerIO);
    DeleteMsgPort (TimerMsgPort);

    
#warning TODO: Add support for non-CLUT screens
#warning TODO: Use TimerIO (IntuitionBase->TimerIO) instead of self-made MyTimerIO!

    AROS_LIBFUNC_EXIT
} /* DisplayBeep */
