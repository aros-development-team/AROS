
/*
    (C) 1999 - 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <libraries/reqtools.h>
#include <aros/libcall.h>

#include "reqtools_intern.h"
#include "general.h"

/*****************************************************************************

    NAME */

    AROS_LH4(void, rtSetReqPosition,

/*  SYNOPSIS */

	AROS_LHA(ULONG, reqpos, D0),
	AROS_LHA(struct NewWindow *, nw, A0),
	AROS_LHA(struct Screen *, scr, A1),
	AROS_LHA(struct Window *, win, A2),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 21, ReqTools)

/*  FUNCTION
   
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning Taken from rtfuncs.asm where the C version was in comments. Might be out of date

    int mx, my, val, leftedge, topedge;
    ULONG scrwidth, scrheight;
    int width, height, left, top;

    rtGetVScreenSize (scr, &scrwidth, &scrheight);

    leftedge = -scr->LeftEdge;    
    if (leftedge < 0) leftedge = 0;
    
    topedge = -scr->TopEdge;
    if (topedge < 0) topedge = 0;

    left = leftedge; top = topedge;
    width = scrwidth; height = scrheight;
    
    switch (reqpos)
    {
	case REQPOS_DEFAULT:
	    nw->LeftEdge = 25;
	    nw->TopEdge = 18;
	    goto topleftscr;
	    
	case REQPOS_POINTER:
	    mx = scr->MouseX; my = scr->MouseY;
	    break;
	    
	case REQPOS_CENTERWIN:
	    if (win)
	    {
		left = win->LeftEdge; top = win->TopEdge;
		width = win->Width; height = win->Height;
	    }
	    
	case REQPOS_CENTERSCR:
	    mx = (width - nw->Width) / 2 + left;
	    my = (height - nw->Height) / 2 + top;
	    break;
	    
	case REQPOS_TOPLEFTWIN:
	    if (win)
	    {
		left = win->LeftEdge;
		top = win->TopEdge;
	    }
	    
	case REQPOS_TOPLEFTSCR:
topleftscr:
	    mx = left; my = top;
	    break;
    } /* switch (reqpos) */

    /* keep window completely visible */
    mx += nw->LeftEdge; my += nw->TopEdge;
    val = leftedge + scrwidth - nw->Width;
    
    if (mx < leftedge) mx = leftedge;
    else if (mx > val) mx = val;
    
    val = topedge + scrheight - nw->Height;
    
    if (my < topedge) my = topedge;
    else if (my > val) my = val;

    nw->LeftEdge = mx; nw->TopEdge = my;
    
    AROS_LIBFUNC_EXIT
    
} /* rtSetReqPosition */
