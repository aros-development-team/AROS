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

    AROS_LH2(LONG, rtChangeReqAttrA,

/*  SYNOPSIS */

	AROS_LHA(APTR, req, A1),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 8, ReqTools)

/*  FUNCTION
	Change requester attributes with supplied taglist. This is the only
	correct way to change the attributes listed below.

	The return code from rtChangeReqAttrA() should be ignored unless
	stated otherwise.

	Don't pass the tags listed below to the requester itself (unless
	documented otherwise). They will not be recognized.
   
    INPUTS
	req     - pointer to requester.
	taglist - pointer to array of tags.

    TAGS
	For the file requester:

	RTFI_Dir - (char *)
	    Name of new directory to position file requester in. The
	    requester's buffer will be deallocated.

	RTFI_MatchPat - (char *) New pattern string to match files on.

	RTFI_AddEntry - (BPTR) THIS *MUST* BE THE LAST TAG (just before
	    TAG_END)! Tagdata must hold a lock on a file or directory you
	    want to add to the file requester's buffer. The lock should
	    have been obtained using Lock(), and you must unlock this lock
	    yourself. It is your responsibility to make sure the file or
	    directory is indeed in the directory the file requester is in.
	    If the entry is already in the file requester's buffer it will
	    simply be updated.
	    It is harmless to use this tag if the requester's buffer is not
	    initialized. rtChangeReqAttr() will return a boolean to
	    indicate success or failure (out of memory).

	RTFI_RemoveEntry - (char *) Name of file or directory you want to
	    remove from the file requester's buffer. It is your
	    responsibility to make sure the file or directory is indeed in
	    the directory the file requester is in.
	    It is harmless use this tag if the requester's buffer is not
	    initialized.

	For the font requester:

	RTFO_FontName - (char *) Set the name of the currently selected
	    font.

	RTFO_FontHeight - (UWORD) Set the fontsize of the currently
	    selected font.

	RTFO_FontStyle - (UBYTE) Set the style of the current font.

	RTFO_FontFlags - (UBYTE) Set the flags of the current font.

	For the screenmode requester [V38]:

	RTSC_ModeFromScreen - (struct Screen *) Screen to get mode
	    attributes from.

	    NOTE: You must make sure the mode this screen is in will be
	        accepted by the screen mode requester. Otherwise it will
	        automatically cancel. For example, you use
	        RTDI_ModeFromScreen on a HAM screen and you haven't set the
	        SCREQF_NONSTDMODES flag.
	        Note that you must use this tag _before_ the four tags
	        below because this tag will set the width, height, depth
	        and autoscroll.

	RTSC_DisplayID - (ULONG) Set 32-bit mode id of selected mode. The
	    width and height will be set to the default (visible) width and
	    height, and the depth will be set to maximum. Also read note
	    above. Note that you must use this tag _before_ the three tags
	    below because this tag will set the width, height and depth to
	    default values.

	RTSC_DisplayWidth - (UWORD) Set width of display. Must come after
	    RTSC_DisplayID or RTSC_ModeFromScreen tags.

	RTSC_DisplayHeight - (UWORD) Set height of display. Must come after
	    RTSC_DisplayID or RTSC_ModeFromScreen tags.

	RTSC_DisplayDepth - (UWORD) Set depth of display. Must come after
	    RTSC_DisplayID or RTSC_ModeFromScreen tags.

	RTSC_AutoScroll - (BOOL) Boolean state of autoscroll checkbox. Must
	    come after RTSC_ModeFromScreen tag.

	RTSC_OverscanType - (ULONG) Set type of overscan. Set to 0 for
	    regular size, otherwise use OSCAN_... constants. See
	    'intuition/screens.[h|i]'.

    RESULT
	none (except when RTFI_AddEntry tag is used, see above)

    NOTES

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	dos.library/Lock()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return ChangeReqAttrA(req, taglist); /* in filereqalloc.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtChangeReqAttrA */
