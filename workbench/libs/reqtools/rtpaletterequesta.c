/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include <aros/libcall.h>

#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH3(LONG, rtPaletteRequestA,

/*  SYNOPSIS */

	AROS_LHA(char *, title, A2),
	AROS_LHA(struct rtReqInfo *, reqinfo, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 17, ReqTools)

/*  FUNCTION
	Put up a palette requester so the user can change the screen's
	colors.

	The colors are changed in the viewport of the screen the requester
	will appear on, so that is where you will find them after the
	palette requester returns.

	The selected color is returned, so you can also use this requester
	to let the user select a color.

	'reqinfo' can be used to customize the requester. For greater
	control use the tags listed below. The advantage of the rtReqInfo
	structure is that it is global, where tags have to be specified
	each function call. See libraries/reqtools.[hi] for a description
	of the rtReqInfo structure.
   
    INPUTS
	title - pointer to requester window title (null terminated).
	reqinfo - pointer to a rtReqInfo structure allocated with
	    rtAllocRequest() or NULL.
	taglist - pointer to a TagItem array.

    TAGS
	RT_Window - see rtEZRequestA()

	RT_ReqPos - see rtEZRequestA()

	RT_LeftOffset - see rtEZRequestA()

	RT_TopOffset - see rtEZRequestA()

	RT_PubScrName - see rtEZRequestA()

	RT_Screen - see rtEZRequestA()

	RT_ReqHandler - see rtEZRequestA()

	RT_WaitPointer - see rtEZRequestA()

	RT_LockWindow - [V38] see rtEZRequestA()

	RT_ScreenToFront - [V38] see rtEZRequestA()

	RT_ShareIDCMP - [V38] see rtEZRequestA()

	RT_Locale - [V38] see rtEZRequestA()

	RT_IntuiMsgFunc - [V38] see rtEZRequestA()

	RT_DefaultFont - (struct TextFont *) This tag allows you to specify
	    the font to be used in the requester when the screen font is
	    proportional. Default is GfxBase->DefaultFont. This tag is
	    obsolete in ReqTools 2.2 and higher, when running OS 3.0 or
	    higher!

	RT_TextAttr - [V38] see rtFileRequestA() If the font is
	    proportional on Kickstart 2.04 or below ReqTools will use the
	    system default font or the font supplied with RT_DefaultFont.
	    On Kickstart 3.0 or higher the proportional font is used.

	RTPA_Color - (ULONG) Initially selected color of palette. Default
	    is 1.

    RESULT
	color - the color number of the selected color or -1 if the user
	    canceled the requester.

    NOTES
	Automatically adjusts the requester to the screen font. On
	Kickstart 2.04 or lower, if the screen font is proportional the
	default font will be used.

	If the requester got too big for the screen because of a very large
	font, the topaz.font will be used.

	rtPaletteRequestA() checks the pr_WindowPtr of your process to find
	the screen to put the requester on.

    EXAMPLE

    BUGS
	none known

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return PaletteRequestA(title, reqinfo, taglist); /* in palettereq.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtPaletteRequestA */
