/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/15 15:45:31  digulla
    Two new functions: LockIBase() and UnlockIBase()
    Modified code to make sure that it is impossible to access illegal data (ie.
    	fields of a window which is currently beeing closed).

    Revision 1.4  1996/09/21 15:54:21  digulla
    Use Screens' font if there is one

    Revision 1.3  1996/09/21 14:20:26  digulla
    DEBUG Code
    Initialize new RastPort with InitRastPort()

    Revision 1.2  1996/08/29 13:33:32  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.1  1996/08/13 15:37:27  digulla
    First function for intuition.library


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>

#ifndef DEBUG_OpenWindow
#   define DEBUG_OpenWindow 0
#endif
#if DEBUG_OpenWindow
#   undef DEBUG
#   define DEBUG 1
#endif
#include <aros/debug.h>

extern int intui_OpenWindow (struct Window *,
	    struct IntuitionBase *);
extern int intui_GetWindowSize (void);

/*****************************************************************************

    NAME */
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	__AROS_LH1(struct Window *, OpenWindow,

/*  SYNOPSIS */
	__AROS_LHA(struct NewWindow *, newWindow, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 34, Intuition)

/*  FUNCTION
	Opens a new window with the characteristiks specified in
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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
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
    rp = AllocMem (sizeof (struct RastPort), MEMF_ANY);

    if (!w || !rp)
	goto failexit;

    if (!ModifyIDCMP (w, newWindow->IDCMPFlags))
	goto failexit;

    InitRastPort (rp);

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

    if (newWindow->Flags & ACTIVATE)
	IntuitionBase->ActiveWindow = w;

    UnlockIBase (lock);

    RefreshGadgets (w->FirstGadget, w, NULL);

    goto exit;

failexit:
    ModifyIDCMP (w, 0L);

    if (rp)
	FreeMem (rp, sizeof (struct RastPort));

    if (w)
    {
	FreeMem (w, intui_GetWindowSize ());

	w = NULL;
    }

exit:
    ReturnPtr ("OpenWindow", struct Window *, w);
    __AROS_FUNC_EXIT
} /* OpenWindow */
