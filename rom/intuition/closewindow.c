/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    $Log$
    Revision 1.38  2001/10/05 18:42:01  stegerg
    No need for private field "initialfont" in IntWindow structure,
    because there's Window->IFont which can (must) be used instead.

    Revision 1.37  2001/01/27 20:00:31  stegerg
    clear SIGF_INTUITION before using it

    Revision 1.36  2001/01/12 21:25:07  stegerg
    dont CloseFont window->RPort->font but IntWindow->initialfont (the font
    which was opened during OpenWindow)

    Revision 1.35  2000/12/25 22:52:35  bergers
    Bugfix for child windows.

    Revision 1.34  2000/12/25 12:59:42  hkiel
    Immediately return on NULL argument.

    Revision 1.33  2000/12/19 14:24:02  bergers
    Bugfix in unlinking childwindows.

    Revision 1.32  2000/12/16 17:20:42  bergers
    Reintroduced child windows.

    Revision 1.30  2000/12/15 01:11:49  bergers
    Needed to do some more linking around with parent window (if exists).

    Revision 1.29  2000/12/15 00:52:50  bergers
    Added child support for windows.

    Revision 1.28  2000/08/03 20:36:31  stegerg
    screen depth gadget should be usable now + src cleanup + small fixes

    Revision 1.27  2000/08/03 18:30:50  stegerg
    renamed DeferedAction??? to IntuiAction???. The IntuiActionMessage
    structure (formerly called DeferedActionMessage) now contains an
    union for the variables needed by the different actions.

    Revision 1.26  2000/07/08 20:16:17  stegerg
    bugfix (could access memory which it just freed a bit before)

    Revision 1.25  2000/06/06 17:35:24  stegerg
    now opening/closeing windows works also on the input.device task. This
    is needed for things like boopsi popup gadget where one often uses a
    window for the popup menu.

    Revision 1.24  2000/05/30 17:18:25  stegerg
    the descendant/parent list of windows was still not okay although it
    was already fixed several times, including by me. And this stupid
    bug also caused the strange things/crashes in DirectoryOpus when
    for exampling hunting or searching for a file. I had been looking
    for this bug in DirOpus (which as said turned out not to be a bug
    in DirOpus) for many days. Arrgh :-(

    Revision 1.23  2000/04/07 19:44:49  stegerg
    call UnlockPubScreen only if window->MoreFlags has bit
    WMFLG_DO_UNLOCKPUBSCREEN set. Not each window on a public
    screen is a visitor window (for example requester windows
    are usually not) and not every window caues a LockPubScreen
    when it is opened, for example when WA_CustomScreen is used
    (like by Requesters) or WA_PubScreen, <something != NULL>.

    Revision 1.22  2000/02/04 21:56:17  stegerg
    use SendDeferedActionMsg instead of PutMsg

    Revision 1.21  2000/01/21 23:09:14  stegerg
    screen windowlist (scr->FirstWindow, win->NextWindow) was
    not always correct. Maybe (!) it is now.

    Revision 1.20  2000/01/19 19:00:40  stegerg
    moved intui_closewindow from intuition_driver.c to
    here.

    Revision 1.19  1999/10/13 21:07:47  stegerg
    closemessage goes to deferedactionport now

    Revision 1.18  1999/08/20 16:26:21  SDuvan
    Added public screen capabilities

    Revision 1.17  1999/04/02 19:29:32  nlorentz
    CloseWindow() now waits for intuition to close window before it returns

    Revision 1.16  1999/03/26 10:39:29  nlorentz
    Call int_activatewindow() instead of ActivateWindow()

    Revision 1.14  1999/03/19 20:21:35  nlorentz
    Fixed race condition bug between CloseWindow()/inputhandler by doing most of window closing on inputhandlers context. Also Closewindow() was called direcly from inputhandler, that would cause FreeSignal() in DeleteMsgPort() to be called on the wrong task context

    Revision 1.13  1998/12/31 21:43:18  nlorentz
    Bugfix: CloseWindow should no longer free win->RPort as that is done in intui_CloseWindow()

    Revision 1.12  1998/10/20 16:45:53  hkiel
    Amiga Research OS

    Revision 1.11  1998/01/16 23:07:16  hkiel
    Always #undef DEBUG to assure proper behaviour with cpak

    Revision 1.10  1998/01/05 21:06:43  hkiel
    Added masquerade to #include <aros/debug.h> for cpak.

    Revision 1.9  1997/01/27 00:36:36  ldp
    Polish

    Revision 1.8  1996/12/10 14:00:01  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.7  1996/11/08 11:28:00  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.6  1996/10/31 13:50:55  aros
    Don't forget to free the RastPort

    Revision 1.5  1996/10/24 15:51:18  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/10/15 15:45:31  digulla
    Two new functions: LockIBase() and UnlockIBase()
    Modified code to make sure that it is impossible to access illegal data (ie.
	fields of a window which is currently beeing closed).

    Revision 1.3  1996/09/21 14:16:26  digulla
    Debug code
    Only change the ActiveWindow is it is beeing closed
    Search for a new ActiveWindow

    Revision 1.2  1996/08/29 13:33:30  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.1  1996/08/13 15:37:26  digulla
    First function for intuition.library


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include "inputhandler.h"
#include <proto/exec.h>
#include <proto/graphics.h>

#ifndef DEBUG_CloseWindow
#   define DEBUG_CloseWindow 0
#endif
#undef DEBUG
#if DEBUG_CloseWindow
#   define DEBUG 1
#endif
#	include <aros/debug.h>

/******************************************************************************/

#define IW(x) ((struct IntWindow *)x)    
#define MUST_UNLOCK_SCREEN(window,screen) (((GetPrivScreen(screen)->pubScrNode != NULL) && \
    		       (window->MoreFlags & WMFLG_DO_UNLOCKPUBSCREEN)) ? TRUE : FALSE)
		       
void LateCloseWindow(struct MsgPort *userport,
		     struct Screen *screen, BOOL do_unlockscreen,
		     struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, CloseWindow,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 12, Intuition)

/*  FUNCTION
	Closes a window. Depending on the display, this might not happen
	at the time when this function returns, but you must not use
	the window pointer after this function has been called.

    INPUTS
	window - The window to close

    RESULT
	None.

    NOTES
	The window might not have been disappeared when this function returns.

    EXAMPLE

    BUGS

    SEE ALSO
	OpenWindow(), OpenWindowTags()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    
    struct IntuiActionMessage 	*msg;
    
    struct IIHData 		*iihd;
    
    struct MsgPort 		*userport;
    struct Screen 		*screen;
    BOOL 			do_unlockscreen;

    D(bug("CloseWindow (%p)\n", window));

    if ( window == NULL )
    {
      ReturnVoid ("CloseWindow");
    }

    iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    
    screen = window->WScreen;
    do_unlockscreen = MUST_UNLOCK_SCREEN(window, screen);
    
    /* We take a very simple approach to avoid race conditions with the
       intuition input handler running one input.device 's task:
       We just send it a msg about closing the window
    */
    if (HAS_CHILDREN(window))
    {
      struct Window * cw = window->firstchild;
      /*
       * First close all its children, we could also return 
       * a failure value which newer apps that use child windows
       * have to look at.
       */
      while (cw)
      {
        struct Window * _cw;
        _cw = cw->nextchild;
        CloseWindow(cw);
        cw = _cw;
      }
      
    }
    
    if (IS_CHILD(window))
    {
      /*
       * Unlink the window from its parent or
       * out of the list of child windows.
       */
      if (window->parent->firstchild == window)
        window->parent->firstchild = window->nextchild;
      else
        window->prevchild->nextchild = window->nextchild;

      if (window->nextchild)
        window->nextchild->prevchild = window->prevchild;
         
    }

    msg = IW(window)->closeMessage;
    msg->Task = FindTask(NULL); /* !! */
    
    /* We must save this here, because after we have returned from
       the Wait() the window is gone  */
    userport = window->UserPort;

    SetSignal(0, SIGF_INTUITION); /* !! */
    
    SendIntuiActionMsg(msg, IntuitionBase);

    /* Attention: a window can also be created on the input device task context,
       usually (only?) for things like popup gadgets. */
       
    if (FindTask(NULL) != iihd->InputDeviceTask) /* don't use msg->Task instead of FindTask(NULL) here!! */
    {

	/* We must use a bit hacky way to wait for intuition
	  to close the window. Since there may be no userport
	  at this point, we can't wait for a message so we must wait for a
	  system signal instead
	*/

	Wait(SIGF_INTUITION);
        LateCloseWindow(userport, screen, do_unlockscreen, IntuitionBase);
    }
        
    ReturnVoid ("CloseWindow");
    AROS_LIBFUNC_EXIT
} /* CloseWindow */

/******************************************************************************/

void LateCloseWindow(struct MsgPort *userport,
		     struct Screen *screen, BOOL do_unlockscreen,
		     struct IntuitionBase *IntuitionBase)
{
    if (do_unlockscreen) UnlockPubScreen(NULL, screen);

    /* As of now intuition has removed us from th list of
       windows, and we will recieve no more messages
    */
    
    if (userport)
    {
	struct IntuiMessage *im;
	
    	while ((im = (struct IntuiMessage *) GetMsg (userport)))
	    ReplyMsg ((struct Message *)im);
	    

	/* Delete message port */
	DeleteMsgPort (userport);
    }    
}		     


/******************************************************************************/

/* This is called from the intuition input handler */
VOID int_closewindow(struct IntuiActionMessage *msg, struct IntuitionBase *IntuitionBase)
{
    /* Free everything except the applications messageport */
    ULONG 		lock;
    
    struct Window 	*window, *win2;
    struct Screen 	*screen;
    struct MsgPort 	*userport;
    struct IIHData 	*iihd;
    struct Task		*msgtask;
    BOOL 		do_unlockscreen;
    
    D(bug("CloseWindow (%p)\n", window));

    window = msg->Window;
    msgtask = msg->Task;
    
    /* Need this in case of a window created under the input.device task context */
    screen = window->WScreen;
    userport = window->UserPort;
    do_unlockscreen = MUST_UNLOCK_SCREEN(window, screen);
    
    iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    lock = LockIBase (0);
    
    if (window == IntuitionBase->ActiveWindow)
	IntuitionBase->ActiveWindow = NULL;


    /* Remove window from the parent/descendant chain and find next active window
    **
    **
    ** before:  parent win xyz
    **              \
    ** 	             \
    **	         deadwindow window
    **	            /
    **	           /
    **	          /
    **	    descendant win abc 
    **
    ** after: parent win xyz
    **            |
    **            |
    **            |
    **        descendant win abc
    **
    */

    if (window->Descendant)
    {
	window->Descendant->Parent = window->Parent;
    }
    if (window->Parent)
    {
	window->Parent->Descendant = window->Descendant;

	if (!IntuitionBase->ActiveWindow && window->Parent)
	    int_activatewindow (window->Parent, IntuitionBase);
    }

    /* Make sure the Screen's window list is still valid */
    
    if (window == window->WScreen->FirstWindow)
    {
	window->WScreen->FirstWindow = window->NextWindow;
    }
    else if ((win2 = window->WScreen->FirstWindow))
    {
	while (win2->NextWindow)
	{
            if (win2->NextWindow == window)
	    {
		win2->NextWindow = window->NextWindow;
		break;
	    }
	    win2 = win2->NextWindow;
	}
    }
    
    UnlockIBase (lock);

    /* Free resources */
    
    CloseFont (window->IFont);
    
    /* Let the driver clean up. Driver wil dealloc window's rastport */
    intui_CloseWindow (window, IntuitionBase);

    /* msg param now longer valid ... */
    
    if (IW(window)->closeMessage)
	FreeIntuiActionMsg(IW(window)->closeMessage, IntuitionBase);

    /* ... after the FreeMem above!! */
    
    /* Free memory for the window */
    FreeMem (window, sizeof(struct IntWindow));
    
    if (msgtask != iihd->InputDeviceTask)
    {
	/* All done. signal caller task that it may proceed */
	Signal(msgtask, SIGF_INTUITION);
    } else {
        LateCloseWindow(userport, screen, do_unlockscreen, IntuitionBase);
    }
    
    return;
    
} /* int_closewindow */


/**********************************************************************************/


void intui_CloseWindow (struct Window * w,
	                struct IntuitionBase * IntuitionBase)
{
    KillWinSysGadgets(w, IntuitionBase);
    
    if (0 == (w->Flags & WFLG_GIMMEZEROZERO))
    {
	/* not a GZZ window */
	if (w->WLayer)
      	    DeleteLayer(0, w->WLayer);
	DeinitRastPort(w->BorderRPort);
	FreeMem(w->BorderRPort, sizeof(struct RastPort));
    }
    else
    {
      /* a GZZ window */
      /* delete inner window */
      if (NULL != w->WLayer)
          DeleteLayer(0, w->WLayer);
      
      /* delete outer window */
      if (NULL != w->BorderRPort && 
          NULL != w->BorderRPort->Layer)
          DeleteLayer(0, w->BorderRPort->Layer);      
    }
}

