/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include "intuition_intern.h"

#define BEEPTICKS       3   /* in 1/10th sec units */

static inline ULONG beepcolor(ULONG x)
{
    return x < 0x88888888 ? 0xCCCCCCCC : 0x55555555;
}


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

#if USE_NEWDISPLAYBEEP

    BOOL AllScreens = FALSE;
    ULONG lock;

    //dprintf("INT_DisplayBeep: entry, screen 0x%lx\n", (ULONG) screen);

#ifdef USEWINDOWLOCK
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#else
    return; //RenderScreenBar NOT safe without this
#endif
    //jDc: screenlist will NOT be modified while we hold WINDOWLOCK!
    //lock = LockIBase(0);

    /* Start Beep */
    if (screen == (struct Screen *) -1 || screen == NULL)
    {
        screen = IntuitionBase->FirstScreen;
        AllScreens = TRUE;
    }

    while (screen)
    {
        //dprintf("INT_DisplayBeep: screen 0x%lx\n", (ULONG) screen);

        if (screen->Flags & BEEPING)
        {
            GetPrivScreen(screen)->BeepingCounter = BEEPTICKS;
        }
        else
        {
            screen->Flags |= BEEPING;

            RenderScreenBar(screen, FALSE, IntuitionBase);
            //dprintf("INT_DisplayBeep: RenderScreenBar done for screen 0x%lx\n", (ULONG) screen);

            GetPrivIBase(IntuitionBase)->BeepingScreens++;
            GetPrivScreen(screen)->BeepingCounter = BEEPTICKS;
        }

        screen =  AllScreens ? screen->NextScreen : NULL;
    }

    //UnlockIBase(lock);

#ifdef USEWINDOWLOCK
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#endif
    //dprintf("INT_DisplayBeep: done\n");

#else /* USE_NEWDISPLAYBEEP */

    struct MsgPort  	*TimerMsgPort;
    struct timerequest  *BeepTimerIO;
    BOOL    	    	 VisualBeep = TRUE;
    BOOL    	    	 AllScreens = FALSE;
    LONG    	    	 NumBeeped = 0;

    TimerMsgPort = CreateMsgPort();
    if (!TimerMsgPort)
    {
        return;
    }
    
    BeepTimerIO = (struct timerequest *) CreateIORequest (TimerMsgPort, sizeof (struct timerequest));
    if (!BeepTimerIO)
    {
        DeleteMsgPort(TimerMsgPort);
        return;
    }
    
    if (OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest *) BeepTimerIO, 0) != 0)
    {
        DeleteIORequest((struct IORequest *) BeepTimerIO);
        DeleteMsgPort(TimerMsgPort);
        return;
    }

    BeepTimerIO->tr_node.io_Command = TR_ADDREQUEST;
    BeepTimerIO->tr_time.tv_secs    = 0;
    BeepTimerIO->tr_time.tv_micro   = 200000; // 250000


    /* Start Beep */
    if (VisualBeep)
    {
        if (screen == (struct Screen *) -1 || screen == NULL)
        {
            //LockIBase();
            screen = IntuitionBase->FirstScreen;
            //UnlockIBase();
            AllScreens = TRUE;
        }

        do
        {
            Forbid();
            if (!(screen->Flags & BEEPING))
            {
                screen->Flags |= BEEPING;
                Permit();

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

                    GetRGB32 (screen->ViewPort.ColorMap, 0, 1, GetPrivScreen(screen)->DisplayBeepColor0);
                    screen->SaveColor0 = GetRGB4 (screen->ViewPort.ColorMap, 0);

                    SetRGB32 (&screen->ViewPort, 0,
                              beepcolor(GetPrivScreen(screen)->DisplayBeepColor0[0]),
                              beepcolor(GetPrivScreen(screen)->DisplayBeepColor0[1]),
                              beepcolor(GetPrivScreen(screen)->DisplayBeepColor0[2])
                             );

                    // FreeScreenDrawInfo (screen, DInfo);
                }

                else
                    // visual beep on hi- and truecolor screens
                {
                    // struct Window *BeepWindow;
                    struct TagItem window_tags[] =
                    {
                        {WA_Left    	, 0 	    	    	    	    	},
                        {WA_Top     	, 0 	    	    	    	    	},
                        {WA_Width   	, -1	    	    	    	    	},
                        {WA_Height  	, -1	    	    	    	    	},
                        {WA_Flags   	, WFLG_SIMPLE_REFRESH | WFLG_BORDERLESS },
                        {WA_CustomScreen, (IPTR) screen     	    	    	},
                      //{WA_Priority	, 50	    	    	    	    	},  // Place in front of all other windows!
                        {TAG_DONE   	    	    	    	    	    	}
                    };

                    GetPrivScreen(screen)->DisplayBeepWindow = (struct Window *) OpenWindowTagList (NULL, window_tags);
                    screen->Flags |= BEEPING;
                }
                NumBeeped++;

                //LockIBase();
                if (AllScreens) screen = screen->NextScreen;
                //UnlockIBase();
            }
            else
                Permit();

        }
        while (AllScreens && screen);
    }

    if (NumBeeped)
    {
        /* Wait */
        DoIO ((struct IORequest *) BeepTimerIO);
    }

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
            Forbid();
            if (screen->Flags & BEEPING)
            {
                Permit();

                if (GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) <= 8)
                    // visual beep on CLUT-screen
                {
                    // SetRGB4 (&screen->ViewPort, 0, screen->SaveColor0 & 0x000F, (screen->SaveColor0 & 0x00F0) >> 4, (screen->SaveColor0 & 0x0F00) >> 8);
                    SetRGB32 (&screen->ViewPort, 0,
                              GetPrivScreen(screen)->DisplayBeepColor0[0],
                              GetPrivScreen(screen)->DisplayBeepColor0[1],
                              GetPrivScreen(screen)->DisplayBeepColor0[2]
                             );
                }
                else
                    // visual beep on hi- and truecolor screens
                {
                    if (GetPrivScreen(screen)->DisplayBeepWindow)
                    {
                        CloseWindow (GetPrivScreen(screen)->DisplayBeepWindow);
                        GetPrivScreen(screen)->DisplayBeepWindow = NULL;
                    }
                }

                screen->Flags &= (UWORD) ~BEEPING;
            }
            else
                Permit();

            //LockIBase();
            if (AllScreens) screen = screen->NextScreen;
            //UnlockIBase();

        }
        while (AllScreens && screen);
    }

    CloseDevice((struct IORequest *) BeepTimerIO);
    DeleteIORequest((struct IORequest *) BeepTimerIO);
    DeleteMsgPort(TimerMsgPort);

    #warning TODO: Make this "multitasking proof"!
    #warning TODO: Produce beep according to prefs
    #warning TODO: Check if BEEPING flag is handled correctly
    #warning TODO: Make BeepWindow 50% transparent! :-)
    #warning TODO: Use TimerIO (IntuitionBase->TimerIO) instead of self-made BeepTimerIO?

#endif /* USE_NEWDISPLAYBEEP */

    AROS_LIBFUNC_EXIT
} /* DisplayBeep */
