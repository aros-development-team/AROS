/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

    w  = AllocMem (intui_GetWindowSize (), MEMF_CLEAR);
    rp = AllocMem (sizeof (struct RastPort), MEMF_CLEAR);

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

    SetFont (rp, GfxBase->DefaultFont);
    SetAPen (rp, newWindow->DetailPen);
    SetBPen (rp, newWindow->BlockPen);
    SetDrMd (rp, JAM2);

    SetWindowTitles (w, newWindow->Title, (STRPTR)-1);

    w->Parent = NULL;
    w->NextWindow = w->Descendant = w->WScreen->FirstWindow;
    w->WScreen->FirstWindow = w;

    if (newWindow->Flags & ACTIVATE)
	IntuitionBase->ActiveWindow = w;

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
    return w;
    __AROS_FUNC_EXIT
} /* OpenWindow */
