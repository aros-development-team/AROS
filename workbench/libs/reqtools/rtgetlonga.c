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
#include <libraries/reqtools.h>
#include <aros/libcall.h>

#include "reqtools_intern.h"
#include "general.h"

/*****************************************************************************

    NAME */

    AROS_LH4(ULONG, rtGetLongA,

/*  SYNOPSIS */

	AROS_LHA(ULONG *, longptr, A1),
	AROS_LHA(char *, title, A2),
	AROS_LHA(struct rtReqInfo *, reqinfo, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 13, ReqTools)

/*  FUNCTION
	Puts up a requester to get a signed long (32-bit) number from the
	user.

	'reqinfo' can be used to customize the requester. For greater
	control use the tags listed below. The advantage of the rtReqInfo
	structure is that it is global, where tags have to be specified
	each function call. See libraries/reqtools.[hi] for a description
	of the rtReqInfo structure.
   
    INPUTS
	&longvar - address of long (32 bit!) variable to hold result.
	title - pointer to null terminated title of requester window.
	reqinfo - pointer to a rtReqInfo structure allocated with
	    rtAllocRequest() or NULL.
	taglist  - pointer to a TagItem array.

    TAGS
	RT_Window - see rtEZRequestA()

	RT_IDCMPFlags - see rtEZRequestA()

	RT_ReqPos - see rtEZRequestA()

	RT_LeftOffset - see rtEZRequestA()

	RT_TopOffset - see rtEZRequestA()

	RT_PubScrName - see rtEZRequestA()

	RT_Screen - see rtEZRequestA()

	RT_ReqHandler - see rtEZRequestA()

	RT_WaitPointer - see rtEZRequestA()

	RT_Underscore - [V38] see rtEZRequestA() Only when you also use the
	    RTGL_GadFmt tag.

	RT_LockWindow - [V38] see rtEZRequestA()

	RT_ScreenToFront - [V38] see rtEZRequestA()

	RT_ShareIDCMP - [V38] see rtEZRequestA()

	RT_Locale - [V38] see rtEZRequestA()

	RT_IntuiMsgFunc - [V38] see rtEZRequestA()

	RT_TextAttr - [V38] see rtEZRequestA(). Note that under 1.2/1.3 the
	    string gadget's font will remain the screen font.

	RTGL_Min - (ULONG) Minimum allowed value. If the user tries to
	    enter a smaller value the requester will refuse to accept it.

	RTGL_Max - (ULONG) Maximum allowed value, higher values are refused.

	RTGL_Width - (ULONG) Width of requester window in pixels. This is
	    only a suggestion. rtGetLongA() will not go below a certain
	    width.

	RTGL_ShowDefault - (BOOL) If this is TRUE (default) the value
	    already in 'longvar' will be displayed in the requester when it
	    comes up. If set to FALSE the requester will be empty.

	RTGL_GadFmt - (char *) [V38] Using this tag you can offer the user
	    several responses. See rtEZRequestA() for more information.
	    Note that selecting this gadget is considered a positive
	    response so the integer in the gadget is copied to '&longvar'.

	RTGL_GadFmtArgs - (APTR) [V38] If you used formatting codes with
	    RTGL_GadFmt use this tag to pass the arguments.

	RTGL_Invisible - (BOOL) [V38] Using this tag you can switch on
	    invisible typing. Very useful if you need to get something like
	    a code number from the user. It is strongly advised to use
	    { RTGL_ShowDefault, FALSE } or the user may get very confused! 
	    Default is FALSE.

	RTGL_BackFill - (BOOL) [V38] Backfill requester window with
	    pattern. Default TRUE.

	RTGL_TextFmt - (char *) [V38] Print these lines of text above the
	    gadget in the requester. Very useful to inform the user of what
	    he should enter. Most of the time you will also want to set the
	    GLREQF_CENTERTEXT flag. If you set the RTGL_BackFill tag to
	    FALSE _no_ recessed border will be placed around the text.
	    Formatting codes may be used in the string (see
	    RTGL_TextFmtArgs tag).

	RTGL_TextFmtArgs - (APTR) [V38] If you used formatting codes with
	    RTGL_TextFmt use this tag to pass the arguments.

	RTGL_Flags - (ULONG) [V38]

	    GLREQF_CENTERTEXT - centers each line of text above the gadget
	        in the requester window. Should be generally set.

	    GLREQF_HIGHLIGHTTEXT - Highlight text above the gadget. You
	        will normally only want to use this if you also turned off
	        the window backfilling.

    RESULT
	ret - TRUE if user entered a number, FALSE if not. If one of your
	    idcmp flags caused the requester to end 'ret' will hold this
	    flag. If you used the RTGL_GadFmt tag the return code will hold
	    the value of the response as with rtEZRequestA().

    NOTES
	'longvar' will NOT change if the requester is aborted.

	Automatically adjusts the requester to the screen font.

	rtGetLongA() checks the pr_WindowPtr of your process to find the
	screen to put the requester on.

	If you use the RTGL_GadFmt tag the return value is not always the
	gadget the user selected. If the integer gadget is empty and the
	user presses the leftmost gadget (normally 'Ok') rtGetLong() will
	return 0 (FALSE)! If the integer gadget is empty and the user
	presses one of the other gadgets rtGetLong() _will_ return its
	value!
	Important: &longvar will not be changed in either of these cases.

    EXAMPLE

    BUGS
	none known

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
   
    return GetString(NULL,
    		     0,
		     title,
		     0,
		     longptr,
		     ENTER_NUMBER,
		     reqinfo,
		     taglist);
		     
    AROS_LIBFUNC_EXIT
  
} /* rtGetLongA */
