/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function OpenWindow()
    Lang: english
*/
#include "intuition_intern.h"
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include "boopsigadgets.h"
#include <exec/ports.h>

#ifndef DEBUG_OpenWindow
#   define DEBUG_OpenWindow 0
#endif
#undef DEBUG
#if DEBUG_OpenWindow
#   define DEBUG 1
#endif
#	include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH1(struct Window *, OpenWindow,

/*  SYNOPSIS */
	AROS_LHA(struct NewWindow *, newWindow, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 34, Intuition)

/*  FUNCTION
	Opens a new window with the characteristics specified in
	newWindow.

    INPUTS
	newWindow - How you would like your new window.

    RESULT
	A pointer to the new window or NULL if it couldn't be
	opened. Reasons for this might be lack of memory or illegal
	attributes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseWindow(), ModifyIDCMP()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    struct Window * w;
    struct RastPort * rp;
    ULONG lock;
    BOOL driver_init_done = FALSE;

    D(bug("OpenWindow (%p = { Left=%d Top=%d Width=%d Height=%d })\n"
	, newWindow
	, newWindow->LeftEdge
	, newWindow->TopEdge
	, newWindow->Width
	, newWindow->Height
    ));

#define IW(x) ((struct IntWindow *)x)

    w  = AllocMem (intui_GetWindowSize (), MEMF_CLEAR);
    
/* nlorentz: For now, creating a rastport becomes the responiibility of
   intui_OpenWindow(). This is because intui_OpenWindow() in
   config/hidd/intuition_driver.c must call CreateUpfrontLayer(),
   and that will create a rastport for the layer/window, and we don't
   want two rastports pr. window.
   Alternatively we may create a layers_driver.c driver for layers,
   and then call CreateUpfrontLayer() here from openwindow.
   For the Amiga window<-->X11 window stuff, the layers driver
   would just allocate a layer struct, a rastport and
   put the rasport into layer->RastPort, so we
   could get it inside this routine and put it into
   window->RPort;.
   
*/    

    if (NULL == w)
    	goto failexit;

    w->UserPort = CreateMsgPort();
    if (NULL == w->UserPort)
	goto failexit;

    if (!ModifyIDCMP (w, newWindow->IDCMPFlags))
	goto failexit;
	
    IW(w)->closeMessage = AllocMem(sizeof (struct closeMessage), MEMF_PUBLIC);
    if (NULL == IW(w)->closeMessage)
    	goto failexit;

    D(bug("modified IDCMP\n"));

    w->LeftEdge    = newWindow->LeftEdge;
    w->TopEdge	   = newWindow->TopEdge;
    w->Width	   = newWindow->Width;
    w->Height	   = newWindow->Height;

    ((struct IntWindow *)w)->ZipLeftEdge = w->LeftEdge;
    ((struct IntWindow *)w)->ZipTopEdge  = w->TopEdge;
    ((struct IntWindow *)w)->ZipWidth    = w->Width;
    ((struct IntWindow *)w)->ZipHeight   = w->Height;

/*    w->RPort	   = rp; */

    w->FirstGadget = newWindow->FirstGadget;


    D(bug("Window dims: (%d, %d, %d, %d)\n"
    	, w->LeftEdge, w->TopEdge, w->Width, w->Height));
	

    if (newWindow->DetailPen == 0xFF) newWindow->DetailPen = 1;
    if (newWindow->BlockPen  == 0xFF) newWindow->BlockPen = 0;

    w->MinWidth  = newWindow->MinWidth;
    w->MinHeight = newWindow->MinHeight;
    w->MaxWidth  = newWindow->MaxWidth;
    w->MaxHeight = newWindow->MaxHeight;

    if (newWindow->Type == PUBLICSCREEN)
	w->WScreen = IntuitionBase->ActiveScreen;
    else if (newWindow->Type == CUSTOMSCREEN)
	w->WScreen = newWindow->Screen;
    else
	w->WScreen = GetPrivIBase (IntuitionBase)->WorkBench;


    /* Copy flags */
    w->Flags = newWindow->Flags;

    if (!(w->Flags & WFLG_BORDERLESS))
    {
	w->BorderLeft   = w->WScreen->WBorLeft;
	w->BorderRight  = w->WScreen->WBorRight;
	w->BorderTop    = w->WScreen->WBorTop;
	w->BorderBottom = w->WScreen->WBorBottom;
    }
    
    if (newWindow->Title || (w->Flags & (WFLG_DRAGBAR | WFLG_CLOSEGADGET | WFLG_DEPTHGADGET)))
    {
    	/* this is a hack. the correct way to "correct" (increase if necessary)
	   the w->Border??? items would be to check all GACT_???BORDER gadgets
	   (inclusive sysgadgets which are GACT_????BORDER gadgets as well) in
	   newwindow->FirstGadget (or WA_Gadgets tag) and all sysgadgets and then
	   make sure that each window border is big enough so that none of these
	   gadgets extends outside the window border area */
	   
    	/* Georg Steger: ??? font ??? */
    	if (w->WScreen->Font)
	    w->BorderTop += ((struct IntScreen *)(w->WScreen))->DInfo.dri_Font->tf_YSize + 1;
	else
	    w->BorderTop += GfxBase->DefaultFont->tf_YSize + 1;
    }
    
    if (!intui_OpenWindow (w, IntuitionBase, newWindow->BitMap))
	goto failexit;

/* nlorentz: The driver has in some way or another allocated a rastport for us,
   which now is ready for us to use. */
   driver_init_done = TRUE;
   rp = w->RPort;

    D(bug("called driver, rp=%p\n", rp));
    if (w->WScreen->Font)
	SetFont (rp, ((struct IntScreen *)(w->WScreen))->DInfo.dri_Font);
    else
	SetFont (rp, GfxBase->DefaultFont);

    D(bug("set fonts\n"));

    SetAPen (rp, newWindow->DetailPen);
    SetBPen (rp, newWindow->BlockPen);
    SetDrMd (rp, JAM2);

    D(bug("set pens\n"));
    SetWindowTitles (w, newWindow->Title, (STRPTR)-1);
    D(bug("set title\n"));

    lock = LockIBase (0);

    w->Parent = NULL;
    w->NextWindow = w->Descendant = w->WScreen->FirstWindow;
    if (w->Descendant)
    {
    	w->Descendant->Parent = w;
    }
    w->WScreen->FirstWindow = w;
    
    w->WindowPort = GetPrivIBase(IntuitionBase)->IntuiReplyPort;



    UnlockIBase (lock);

    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    DoGMLayout(w->FirstGadget, w, NULL, -1, TRUE, IntuitionBase);

    if (NULL != w->FirstGadget)
      RefreshGadgets (w->FirstGadget, w, NULL);
    
    /* !!! This does double refreshing as the system gadgets also are refreshed
       in the above RfreshGadgets() call */

    if (newWindow->Flags & WFLG_ACTIVATE)
    {
    	/* RefreshWindowFrame() will be called from within ActivateWindow().
	No point in doing double refreshing.
	
	!!! NOTE !!! If OpenWindow() is sometime in the future moved
	to input.device's context, one should call int_activatewindow
	instead. */
	
	ActivateWindow(w);
    }
    else
    {

	RefreshWindowFrame(w);
    }
    goto exit;

failexit:
    D(bug("fail\n"));
    
    if (w)
    {

	ModifyIDCMP (w, 0L);


/* nlorentz: Freeing the rasport is now intui_CloseWindow()'s task.

    if (rp)
    {
	FreeRastPort (rp);
    }
*/

	if (IW(w)->closeMessage)
	    FreeMem(IW(w)->closeMessage, sizeof (struct closeMessage));

	if (driver_init_done)
	    intui_CloseWindow(w, IntuitionBase);

	if (w->UserPort)
	    DeleteMsgPort(w->UserPort);
	    
	FreeMem (w, intui_GetWindowSize ());

	w = NULL;
    }

exit:
    ReturnPtr ("OpenWindow", struct Window *, w);
    AROS_LIBFUNC_EXIT
} /* OpenWindow */
