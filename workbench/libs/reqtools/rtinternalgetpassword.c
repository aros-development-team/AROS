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

    AROS_LH5(BOOL, rtInternalGetPasswordA,

/*  SYNOPSIS */

	AROS_LHA(UBYTE *, buffer, A1),
	AROS_LHA(ULONG, checksum, D1),
	AROS_LHA(PWCALLBACKFUNPTR, pwcallback, D2),
	AROS_LHA(struct rtReqInfo *, reqinfo, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 14, ReqTools)

/*  FUNCTION
	Puts up a requester to verify a password from the user.

	'reqinfo' can be used to customize the requester. For greater
	control use the tags listed below. The advantage of the rtReqInfo
	structure is that it is global, where tags have to be specified
	each function call. See libraries/reqtools.[hi] for a description
	of the rtReqInfo structure.
   
    INPUTS
	buffer - pointer to buffer to hold characters entered.
	checksum - checksum of the password, passed to pwcallback.
	    really just the least significant 16 bits.
	pwcallback - pointer to password callback function.
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

	RT_Underscore - [V38] see rtEZRequestA(). Only when you also use
	    the RTGS_GadFmt tag.

	RT_LockWindow - [V38] see rtEZRequestA()

	RT_ScreenToFront - [V38] see rtEZRequestA()

	RT_ShareIDCMP - [V38] see rtEZRequestA()

	RT_Locale - [V38] see rtEZRequestA()

	RT_IntuiMsgFunc - [V38] see rtEZRequestA()

	RT_TextAttr - [V38] see rtEZRequestA(). Note that under 1.2/1.3 the
	    string gadget's font will remain the screen font.

	RTGS_Width - (ULONG) Width of requester window in pixels. This is
	    only a suggestion. rtGetStringA() will not go below a certain
	    width.

	RTGS_AllowEmpty - (BOOL) If RTGS_AllowEmpty is TRUE an empty string
	    will also be accepted and returned. Defaults to FALSE, meaning
	    that if the user enters an empty string the requester will be
	    canceled.

	RTGS_GadFmt - (char *) [V38] Using this tag you can offer the user
	    severalresponses. See rtEZRequestA() for more information. Note
	    that selecting this gadget is considered a positive response so
	    the string in the gadget is copied to 'buffer'.

	RTGS_GadFmtArgs - (APTR) [V38] If you used formatting codes with
	    RTGS_GadFmt use this tag to pass the arguments.

	RTGS_Invisible - (BOOL) [V38] Using this tag you can switch on
	    invisible typing. Very useful if you need to get something like
	    a password from the user. It is strongly advised to use an
	    empty initial string or the user may get very confused! 
	    Default is FALSE.

	RTGS_BackFill - (BOOL) [V38] Backfill requester window with
	    pattern. Default TRUE.

	RTGS_TextFmt - (char *) [V38] Print these lines of text above the
	    gadget in the requester. Very useful to inform the user of what
	    he should enter. Most of the time you will also want to set the
	    GSREQF_CENTERTEXT flag. If you set the RTGS_BackFill tag to
	    FALSE _no_ recessed border will be placed around the text.
	    Formatting codes may be used in the string (see
	    RTGS_TextFmtArgs tag).

	RTGS_TextFmtArgs - (APTR) [V38] If you used formatting codes with
	    RTGS_TextFmt use this tag to pass the arguments.

	RTGS_Flags - (ULONG) [V38]

	    GSREQF_CENTERTEXT - centers each line of text above the gadget
	        in the requester window. Should be generally set.

	    GSREQF_HIGHLIGHTTEXT - Highlight text above the gadget. You
	        will normally only want to use this if you also turned off
	        the window backfilling.

    RESULT
	ret - TRUE if user entered something, FALSE if not. If one of your
	    idcmp flags caused the requester to end 'ret' will hold this
	    flag. If you used the RTGS_GadFmt tag the return code will hold
	    the value of the response as with rtEZRequestA().

    NOTES
	This function is private, and it should NOT be used by user code
	(Currently it is used by powerpacker.library on native AmigaOS).

	The contents of the buffer will be one nullbyte if the requester is
	aborted. This is different to rtGetStringA. Maximum password length
	is 16 chars, the requester title is always "Password" and
	RTGS_Invisible is TRUE by default.

	Automatically adjusts the requester to the screen font.

	rtInternalGetPasswordA() checks the pr_WindowPtr of your process
	to find the screen to put the requester on.

	If you use the RTGS_GadFmt tag the return value is not always the
	gadget the user selected. If the string gadget is empty and the
	user presses the leftmost gadget (normally 'Ok')
	rtInternalGetPasswordA() will return 0 (FALSE)! If the string gadget
	is empty and the user presses one of the other gadgets
	rtInternalGetPasswordA() _will_ return its value!  Important:
	'buffer' will not be changed in either of these cases.
	If you set the RTGS_AllowEmpty tag to TRUE 'buffer' will always be
	changed of course, and rtGetString() will always return the value
	of the gadget pressed.

    EXAMPLE

    BUGS
	This documentation is intentionally incomplete.

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
   
    buffer[0] = '\0';

    return GetString(buffer,
    		     16,
    		     "Password",
		     checksum & 0xffff,
		     (ULONG *) pwcallback,
		     CHECK_PASSWORD,
		     reqinfo,
		     taglist);

    AROS_LIBFUNC_EXIT
  
} /* rtInternalGetPasswordA */
