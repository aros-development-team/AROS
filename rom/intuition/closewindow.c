/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include "inputhandler.h"
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#ifndef DEBUG_CloseWindow
#   define DEBUG_CloseWindow 0
#endif
#undef DEBUG
#if DEBUG_CloseWindow
#   define DEBUG 1
#endif
#	include <aros/debug.h>

/******************************************************************************/

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
	{
	    /* Prevent inputhandler/HandleIntuiReplyPort() from accessing
	       dead window */
	    im->Class = 0;
	    im->Qualifier = 0;
	    ReplyMsg ((struct Message *)im);
	}   

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

    RemoveResourceFromList(window, RESOURCE_WINDOW, IntuitionBase);
        
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

