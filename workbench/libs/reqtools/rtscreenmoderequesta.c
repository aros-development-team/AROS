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

    AROS_LH3(ULONG, rtScreenModeRequestA,

/*  SYNOPSIS */

	AROS_LHA(struct rtScreenModeRequester *, screenmodereq, A1),
	AROS_LHA(char *, title, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 24, ReqTools)

/*  FUNCTION
	*IMPORTANT* THIS REQUESTER IS ONLY AVAILABLE FROM KICKSTART 2.0
	    ONWARDS! The 1.3 version of ReqTools also contains the
	    screenmode requester, but unless you are running 2.0 or higher
	    it will not come up. So what you essentially have to do is NOT
	    call rtScreenModeRequestA() if your program is running on a
	    machine with Kickstart 1.2/1.3. You can safely call
	    rtScreenModeRequestA() if you are running on a 2.0 machine,
	    even if the user has installed the 1.3 version of ReqTools.

	Get a screen mode from the user.

	The user will be able to pick a screen mode by name, enter the size
	and the number of colors (bitplane depth).

	rtScreenModeRequestA() will call the appropriate 2.0 functions to
	get all the mode's information. If no name has been assigned to the
	mode one will be constructed automatically.

    INPUTS
	screenmodereq - pointer to a struct rtScreenModeRequester allocated
	    with rtAllocRequestA().
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

	RT_LockWindow - see rtEZRequestA()

	RT_ScreenToFront - see rtEZRequestA()

	RT_ShareIDCMP - see rtEZRequestA()

	RT_Locale - see rtEZRequestA()

	RT_IntuiMsgFunc - (struct Hook *) [V38] The requester will call
	    this hook for each IDCMP message it gets that doesn't belong to
	    its window. Only applies if you used the RT_ShareIDCMP tag to
	    share the IDCMP port with the parent window. Parameters are as
	    follows:

	    A0 - (struct Hook *) your hook
	    A2 - (struct rtScreenModeRequester *) your req
	    A1 - (struct IntuiMessage *) the message

	    After you have finished examining the message and your hook
	    returns, ReqTools will reply the message. So do not reply the
	    message yourself!

	RT_Underscore - (char) [V38] Indicates the symbol that precedes the
	    character in a gadget's label to be underscored. This will also
	    define the keyboard shortcut for this gadget. Currently only
	    needed for RTSC_OkText. Usually set to '_'.

	RT_DefaultFont - (struct TextFont *) This tag allows you to specify
	    the font to be used in the requester when the screen font is
	    proportional. Default is GfxBase->DefaultFont. This tag is
	    obsolete in ReqTools 2.2 and higher.

	RT_TextAttr - [V38] see rtFileRequestA()

	RTSC_Flags - (ULONG) Several flags:

	    SCREQF_OVERSCANGAD - Add an overscan cycle gadget to the
	        requester. After the requester returns you may read the
	        overscan type in 'rq->OverscanType' If this is 0 no
	        overscan is selected (Regular Size), if non-zero it holds
	        one of the OSCAN_... values defined in the include file
	        'intuition/screens.[h|i]'.

	    SCREQF_AUTOSCROLLGAD - Add an autoscroll checkbox gadget to the
	        requester. After the requester returns read
	        'smreq->AutoScroll' to see if the user prefers autoscroll
	        to be on or off.

	    SCREQF_SIZEGADS - Add width and height gadgets to the
	        requester. If you do not add these gadgets the width and
	        height returned will be the default width and height for
	        the selected overscan type.

	    SCREQF_DEPTHGAD - Add a depth slider gadget to the requester.
	        If you do not add a depth gadget, the depth returned will
	        be the maximum depth this mode can be opened in.

	    SCREQF_NONSTDMODES - Include all modes. Unless this flag is set
	        rtScreenModeRequestA() will exclude nonstandard modes.
	        Nonstandard modes are presently HAM and EHB
	        (ExtraHalfBrite). So unless you are picking a mode to do
	        some rendering in leave this flag unset. Without this flag
	        set the mode returned will be a normal bitplaned mode.

	    SCREQF_GUIMODES - Set this flag if you are getting a screen
	        mode to open a user interface screen in. The modes shown
	        will be standard modes with a high enough resolution
	        (minumum 640 pixels). If this flag is set the
	        SCREQF_NONSTDMODES flag is ignored.

	RTSC_Height - (ULONG) Suggested height of screenmode requester
	    window.

	RTSC_OkText - (char *) Replacement text for "Ok" gadget, max 6
	    chars long.

	RTSC_MinWidth - (UWORD) The minimum display width allowed.

	RTSC_MaxWidth - (UWORD) The maximum display width allowed.

	RTSC_MinHeight - (UWORD) The minimum display height allowed.

	RTSC_MaxHeight - (UWORD) The maximum display height allowed.

	RTSC_MinDepth - (UWORD) The minimum display depth allowed. Modes
	    with a minimum display depth lower than this value will not be
	    included in the list.

	RTSC_MaxDepth - (UWORD) The maximum display depth allowed.

	RTSC_PropertyFlags - (ULONG) A mode must have these property flags
	    to be included. Only bits set in RTSC_PropertyMask are
	    considered.

	RTSC_PropertyMask - (ULONG) Mask to apply to RTSC_PropertyFlags to
	    determine which bits to consider. See use of 'newsignals' and
	    'signalmask' in exec.library/SetSignal(). Default is to
	    consider all bits in RTSC_PropertyFlags as significant.

	RTSC_FilterFunc - (struct Hook *) Call this hook for each display
	    mode id in the system's list. Parameters are as follows:

	    A0 - (struct Hook *) your hook
	    A2 - (struct rtScreenModeRequester *) your req
	    A1 - (ULONG) 32-bit extended mode id

	    If your hook returns TRUE the mode will be accepted. If it
	    returns FALSE the mode will be skipped and will not appear in
	    the requester.

    RESULT
	ret - FALSE if the requester was canceled or TRUE if the user
	    selected a screen mode (check 'smreq->DisplayID' for the 32-bit
	    extended display mode, 'smreq->DisplayWidth' and
	    'smreq->DisplayHeight' for the display size,
	    'smreq->DisplayDepth' for the screen's depth).

    NOTES
	Automatically adjusts the requester to the screen font.

	If the requester got too big for the screen because of a very large
	font, the topaz.font will be used.

	rtScreenModeRequest() checks the pr_WindowPtr of your process to
	find the screen to put the requester on.

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	graphics.library/GetDisplayInfoData(), graphics/displayinfo.h,
	exec.library/SetSignal(), Intuition/SA_DisplayID screen tag

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (ULONG)FileRequestA((struct RealFileRequester *)screenmodereq, NULL, title, taglist); /* in filereq.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtScreenModeRequestA */
