/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

    AROS_LH3(ULONG, rtFontRequestA,

/*  SYNOPSIS */

	AROS_LHA(struct rtFontRequester *, fontreq, A1),
	AROS_LHA(char *, title, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 16, ReqTools)

/*  FUNCTION
	Let the user select a font and a style (optional).
   
    INPUTS
	fontreq  - pointer to a struct rtFontRequester allocated with
	    rtAllocRequestA().
	title - pointer to requester window title (null terminated).
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

	RT_IntuiMsgFunc - (struct Hook *) [V38] The requester will call
	    this hook for each IDCMP message it gets that doesn't belong to
	    its window. Only applies if you used the RT_ShareIDCMP tag to
	    share the IDCMP port with the parent window. Parameters are as
	    follows:

	    A0 - (struct Hook *) your hook
	    A2 - (struct rtFontRequester *) your requester
	    A1 - (struct IntuiMessage *) the message

	    After you have finished examining the message and your hook
	    returns, ReqTools will reply the message. So do not reply the
	    message yourself!

	RT_Underscore - (char) [V38] Indicates the symbol that precedes the
	    character in a gadget's label to be underscored. This will also
	    define the keyboard shortcut for this gadget. Currently only
	    needed for RTFO_OkText. Usually set to '_'.

	RT_DefaultFont - (struct TextFont *) This tag allows you to specify
	    the font to be used in the requester when the screen font is
	    proportional. Default is GfxBase->DefaultFont. This tag is
	    obsolete in ReqTools 2.2 and higher.

	RT_TextAttr - [V38] see rtFileRequestA()

	RTFO_Flags - (ULONG) Several flags:

	    FREQF_NOBUFFER - do not buffer the font list for subsequent
	        calls to rtFontRequestA().

	    FREQF_FIXEDWIDTH - only show fixed-width fonts.

	    FREQF_COLORFONTS - show color fonts also.

	    FREQF_CHANGEPALETTE - change the screen's palette to match that
	        of a selected color font.

	    FREQF_LEAVEPALETTE - leave the palette as it is when exiting
	        rtFontRequestA() Useful in combination with
	        FREQF_CHANGEPALETTE.

	    FREQF_SCALE - allow fonts to be scaled when they don't exist in
	        the requested size. (works on Kickstart 2.0 only, has no
	        effect on 1.2/1.3).

	    FREQF_STYLE - include gadgets so the user may select the font's
	        style.

	RTFO_Height - (ULONG) Suggested height of font requester window.

	RTFO_OkText - (char *) Replacement text for "Ok" gadget. Maximum 6
	    chars. (7 is still ok, but not esthetically pleasing)

	RTFO_SampleHeight - (ULONG) Height of font sample display in pixels
	    (default 24).

	RTFO_MinHeight - (ULONG) Minimum font size displayed.

	RTFO_MaxHeight - (ULONG) Maximum font size displayed.

	RTFO_FilterFunc - (struct Hook *) [V38] Call this hook for each
	    available font. Parameters are as follows:

	    A0 - (struct Hook *) your hook
	    A2 - (struct rtFontRequester *) your filereq
	    A1 - (struct TextAttr *) textattr of font

	    If your hook returns TRUE the font will be accepted. If it
	    returns FALSE the font will be skipped and will not appear in
	    the requester. IMPORTANT NOTE:  If you change your hook's
	    behavior you _MUST_ purge the requester's buffer (using
	    rtFreeReqBuffer())!

    RESULT
	bool - TRUE if the user selected a font (freq->Attr holds the
	    font), FALSE if the requester was canceled.

    NOTES
	You CANNOT call the font requester from a task because it may use
	DOS calls!

	Automatically adjusts the requester to the screen font.

	If the requester got too big for the screen because of a very large
	font, the topaz.font will be used.

	rtFontRequest() checks the pr_WindowPtr of your process to find the
	screen to put the requester on.

    EXAMPLE

    BUGS
	none known

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (IPTR)FileRequestA((struct RealFileRequester *)fontreq, NULL, title, taglist); /* in filereq.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtFontRequestA */
