/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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
#include "rtfuncs.h"

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
	Sets newwindow->LeftEdge and newwindow->TopEdge according to reqpos.

	Except for the left- and topedge 'newwindow' must already be
	completely initialized.

	The newwindow->LeftEdge and newwindow->TopEdge already in the
	NewWindow structure will be used as offsets to the requested
	position. If you'd like a window at position (25,18) from the top
	left of the screen you would fill newwindow->LeftEdge with 25,
	newwindow->TopEdge with 18 and call rtSetReqPosition() with reqpos
	equal to REQPOS_TOPLEFTSCR.

	Don't forget to make sure newwindow->LeftEdge and newwindow->TopEdge
	are 0 if you don't want to offset your window.

	In case of REQPOS_POINTER you can use them to point to your window's
	hotspot, where the pointer should point. If you call
	rtSetReqPosition() with the left- and topedge equal to 0 you'd get
	a window appearing with its top- and leftedge equal to the current
	pointer position.

	Note that the screen pointer may _NOT_ be NULL. If you have your
	own window open you can supply yourwindow->WScreen to this function.

	The window pointer is only required if reqpos is REQPOS_CENTERWIN or
	REQPOS_TOPLEFTWIN. Even in this case you may call rtSetReqPosition()
	with a NULL window pointer. The positions will simply fall back to
	REQPOS_CENTERSCR and REQPOS_TOPLEFTSCR respectively.
   
    INPUTS
	reqpos - one of the REQPOS_... constants usable with RT_ReqPos.
	newwindow - pointer to your (already initialized) NewWindow
	    structure.
	screen - pointer to screen the requester will appear on.
	window - pointer to parent window or NULL.

    RESULT
	none

    NOTES
	This function is for the advanced ReqTools user.

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	RT_ReqPos tag

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    RTFuncs_rtSetReqPosition(reqpos, nw, scr, win);
    
    AROS_LIBFUNC_EXIT
    
} /* rtSetReqPosition */
