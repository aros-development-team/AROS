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

    D(bug("OpenWindow (%p = { Left=%d Top=%d Width=%d Height=%d })\n"
	, newWindow
	, newWindow->LeftEdge
	, newWindow->TopEdge
	, newWindow->Width
	, newWindow->Height
    ));

    w  = AllocMem (intui_GetWindowSize (), MEMF_CLEAR);
    rp = CreateRastPort ();

    if (!w || !rp)
	goto failexit;

    if (!ModifyIDCMP (w, newWindow->IDCMPFlags))
	goto failexit;

    w->LeftEdge    = newWindow->LeftEdge;
    w->TopEdge	   = newWindow->TopEdge;
    w->Width	   = newWindow->Width;
    w->Height	   = newWindow->Height;
    w->RPort	   = rp;
    w->FirstGadget = newWindow->FirstGadget;

    if (newWindow->DetailPen == 0xFF) newWindow->DetailPen = 1;
    if (newWindow->BlockPen  == 0xFF) newWindow->BlockPen = 0;

    w->BorderLeft   = 0;
    w->BorderTop    = 0;
    w->BorderRight  = 0;
    w->BorderBottom = 0;

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

    if (!intui_OpenWindow (w, IntuitionBase))
	goto failexit;

    if (w->WScreen->Font)
	SetFont (rp, ((struct IntScreen *)(w->WScreen))->DInfo.dri_Font);
    else
	SetFont (rp, GfxBase->DefaultFont);

    SetAPen (rp, newWindow->DetailPen);
    SetBPen (rp, newWindow->BlockPen);
    SetDrMd (rp, JAM2);

    SetWindowTitles (w, newWindow->Title, (STRPTR)-1);

    lock = LockIBase (0);

    w->Parent = NULL;
    w->NextWindow = w->Descendant = w->WScreen->FirstWindow;
    w->WScreen->FirstWindow = w;

    if (newWindow->Flags & WFLG_ACTIVATE)
	IntuitionBase->ActiveWindow = w;

    UnlockIBase (lock);

    /* Send all GA_RelSpecial BOOPSI gadgets in the list the GM_LAYOUT msg */
    DoGMLayout(w->FirstGadget, w, NULL, -1, TRUE, IntuitionBase);

/*
   remove following line as soon as windows are working with layers.
*/
w->RPort->BitMap->Flags |= BMF_AROS_OLDWINDOW;

    RefreshGadgets (w->FirstGadget, w, NULL);

    goto exit;

failexit:
    ModifyIDCMP (w, 0L);

    if (rp)
	FreeRastPort (rp);

    if (w)
    {
	FreeMem (w, intui_GetWindowSize ());

	w = NULL;
    }

exit:
    ReturnPtr ("OpenWindow", struct Window *, w);
    AROS_LIBFUNC_EXIT
} /* OpenWindow */
