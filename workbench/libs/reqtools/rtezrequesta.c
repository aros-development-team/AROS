/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
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

    AROS_LH5(ULONG, rtEZRequestA,

/*  SYNOPSIS */

	AROS_LHA(char *, bodyfmt, A1),
	AROS_LHA(char *, gadfmt, A2),
	AROS_LHA(struct rtReqInfo *, reqinfo, A3),
	AROS_LHA(APTR, argarray, A4),            /* tagcall: -1 */
	AROS_LHA(struct TagItem *, taglist, A0), /* tagcall: -1 +Tags */ 

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 11, ReqTools)

/*  FUNCTION
	This function puts up a requester for you and waits for a response
	from the user. If the response is positive, this procedure returns
	TRUE. If the response is negative, this procedure returns FALSE.
	The function may also return an IDCMP flag or a value corresponding
	with one of other possible responses (see below).

	'gadfmt' may contain several possible responses. Separate these
	responses by a '|'. For example: "Yes|No", or 'Yes|Maybe|No". The
	responses should be typed in the same order as they will appear on
	screen, from left to right. There is no limit to the number of
	responses other than the width of the screen the requester will
	appear on.

	'bodyfmt' can contain newlines ('\n', ASCII 10). This will cause a
	new line to be started (surprise, surprise :-).
	You may also include printf() style formatting codes. The format
	arguments should be pointed to by 'argarray'.
	You can use formatting codes in 'gadfmt' as well. The arguments for
	this format string should follow the ones for 'bodyfmt'.

	NOTE: The formatting is done by exec.library/RawDoFmt(), so be
	    aware that to display a 32-bit integer argument you must use
	    "%ld", not "%d", since RawDoFmt() is "word-oriented."

	The second and third function use a variable number of arguments.
	These functions can be found in 'reqtools[nb].lib'.
	The second function has the RawDoFmt() arguments as variable args,
	the third the tags. If you need both this is what you can do:

	...
	{
	   ULONG tags[] = { RTEZ_ReqTitle, (ULONG)"mytitle", TAG_END };

	   rtEZRequest ("String, num: %s, %ld", "Ok", NULL,
	                                   (struct TagItem *)tags, "six", 6);
	}
	...

	You can satisfy the requester with the following keyboard shortcuts:
	    'Y' or Left Amiga 'V' for a positive response,
	    ESC, 'N', 'R' or Left Amiga 'B' for a negative response.

	If EZREQF_NORETURNKEY is _not_ set (see RTEZ_Flags below) the
	RETURN key is also accepted as a shortcut for the positive response
	(can be changed using RTEZ_DefaultResponse, see below). The
	response that will be selected when you press RETURN will be
	printed in bold.

	The EZREQF_LAMIGAQUAL flag should be used when you put up a
	requester for a destructive action (e.g. to delete something). When
	it is set the keyboard shortcuts are limited to Left Amiga 'V' and
	'B' so it is harder to accidently select something you will regret.
	Note that the RETURN and ESC key remain active!  To disable the
	RETURN key use the EZREQF_NORETURNKEY flag. The ESC key cannot be
	disabled.

	You may pass a NULL for 'gadfmt', but make sure you know what you
	are doing. Passing a NULL opens an EZRequester with NO responses,
	just a body text. This implies the user has no means of "answering"
	this requester. You must therefore use the RT_IDCMPFlags tag to
	allow some other events to end the requester (e.g.
	IDCMP_MOUSEBUTTONS, IDCMP_INACTIVEWINDOW,...) or you must make use
	of the ReqHandler feature. Using a requester handler you can end
	the requester by program control. This way you can e.g. put up a
	requester before you start loading a file and remove it after the
	file has been loaded. Do not pass an empty string as 'gadfmt'!

	'reqinfo' can be used to customize the requester. For greater
	control use the tags listed below. The advantage of the rtReqInfo
	structure is that it is global, where tags have to be specified
	each function call. See libraries/reqtools.[hi] for a description
	of the rtReqInfo structure.
   
    INPUTS
	bodyfmt  - requester body text, can be format string a la RawDoFmt().
	gadfmt   - text for gadgets (left to right, separated by '|') or NULL.
	argarray - pointer to array of arguments for format string(s).
	reqinfo  - pointer to a rtReqInfo structure allocated with
	    rtAllocRequest() or NULL.
	taglist  - pointer to a TagItem array.

    TAGS
	RT_Window - (struct Window *) Window that will be used to find the
	    screen to put the requester on. You *MUST* supply this if you
	    are a task calling this function and not a process! This is
	    because tasks don't have a pr_WindowPtr.

	RT_IDCMPFlags - (ULONG) Extra idcmp flags to return on. If one
	    these IDCMP flags causes the requester to abort the return code
	    will equal the flag in question.

	RT_ReqPos - (ULONG) One of the following:

	    REQPOS_POINTER - requester appears where the mouse pointer is
	        (default).

	    REQPOS_CENTERSCR - requester is centered on the screen.

	    REQPOS_CENTERWIN - requester is centered in the window (only
	        works if the pr_WindowPtr of your process is valid or if you
	        use RT_Window). If RT_Window is NULL the requester will be
	        centered on the screen.

	    REQPOS_TOPLEFTSCR - requester appears at the top left of the
	        screen.

	    REQPOS_TOPLEFTWIN - requester appears at the top left of the
	        window (only works if the pr_WindowPtr of your process is
	        valid or if you use RT_Window).
	    
	    The requester will always remain in the visible part of the
	    screen, so if you use the Workbench 2.0 ScreenMode preferences
	    editor to enlarge your Workbench screen and you scroll around,
	    the requester will always appear in the part you can see.
	    REQPOS_CENTERSCR and REQPOS_TOPLEFTSCR also apply to the
	    visible part of the screen. So if you use one of these the
	    requester will be appear in the center or the top left off what
	    you can see of the screen as opposed to the entire screen.
	    
	    REQPOS_CENTERWIN and REQPOS_TOPLEFTWIN fall back to
	    REQPOS_CENTERSCR or REQPOS_TOPLEFTSCR respectively when there
	    is no parent window. So you can safely use these without
	    worrying about the existence of a window.

	RT_LeftOffset - (ULONG) Offset of left edge of requester relative to
	    position specified with RT_ReqPos (does not offset the
	    requester when RT_ReqPos is REQPOS_POINTER).

	RT_TopOffset - (ULONG) Offset of top edge of requester relative to
	    position specified with RT_ReqPos (does not offset the
	    requester when RT_ReqPos is REQPOS_POINTER).

	RT_PubScrName - (char *) Name of public screen requester should
	    appear on. When this tag is used the RT_Window tag will be
	    ignored. If the public screen is not found the requester will
	    open on the default public screen.
	    
	    Only works on Kickstart 2.0! reqtools.library does not check
	    this, it is up to you *NOT* to use this tag on Kickstart 1.3 or
	    below! Note that the 1.3 version of reqtools.library also
	    understands and supports this tag (on 2.0).

	RT_Screen - (struct Screen *) Address of screen to put requester
	    on. You should never use this, use RT_Window or RT_PubScrName.

	RT_ReqHandler - (struct rtHandlerInfo **) Using this tag you can
	    start an "asynchronous" requester. ti_TagData of the tag must
	    hold the address of a pointer variable to a rtHandlerInfo
	    structure. The requester will initialize this pointer and will
	    return immediately after its normal initialization. The return
	    code will not be what you would normally expect. If the return
	    code is _not_ equal to CALL_HANDLER an error occurred and you
	    should take appropriate steps.
	    
	    If the return code was CALL_HANDLER everything went ok and the
	    requester will still be up! See the explanation for
	    rtReqHandlerA() below for the following steps you have to take.

	RT_WaitPointer - (BOOL) If this is TRUE the window calling the
	    requester will get a standard wait pointer set while the
	    requester is up. This will happen if you used the RT_Window tag
	    or if your process's pr_WindowPtr is valid. Note that after the
	    requester has finished your window will be ClearPointer()-ed.
	    
	    If you used a custom pointer in your window you will have to
	    re-set it, or not use the RT_WaitPointer tag and put up a wait
	    pointer yourself. If your program requires ReqTools V38 it is
	    advised you use RT_LockWindow instead. Defaults to FALSE.

	RT_LockWindow - (BOOL) [V38] If this is TRUE the window calling the
	    requester will get locked. It will no longer accept any user
	    input and it will get standard wait pointer set. This will
	    happen only if you used the RT_Window tag or if your process's
	    pr_WindowPtr is valid. RT_LockWindow will restore a custom
	    pointer if you have used one (unlike RT_WaitPointer). So you do
	    not have to worry about having to restore it yourself. It is
	    advised you use this tag as much as possible. Defaults to FALSE.

	    Under Kickstart V39 the original window pointer will not be
	    restored if it was set using SetWindowPointer(). You will have
	    to restore the pointer yourself in this case.

	RT_ScreenToFront - (BOOL) [V38] Boolean indicating whether to pop
	    the screen the requester will appear on to the front. Default is
	    TRUE.

	RT_ShareIDCMP - (BOOL) [V38] Boolean indicating whether to share
	    the IDCMP port of the parent window. Use this tag together with
	    the RT_Window tag to indicate the window to share IDCMP with.
	    Sharing the IDCMP port produces less overhead, so it is advised
	    you use this tag. Defaults to FALSE.

	RT_Locale - (struct Locale *) [V38] Locale to determine what
	    language to use for the requester text. If this tag is not used
	    or its data is NULL, the system's current default locale will
	    be used. Default NULL.
	RT_IntuiMsgFunc - (struct Hook *) [V38] The requester will call
	    this hook for each IDCMP message it gets that doesn't belong to
	    its window. Only applies if you used the RT_ShareIDCMP tag to
	    share the IDCMP port with the parent window. Parameters are as
	    follows:
	    
	        A0 - (struct Hook *) your hook
	        A2 - (struct rtReqInfo *) your requester info
	        A1 - (struct IntuiMessage *) the message
	    
	    After you have finished examining the message and your hook
	    returns, ReqTools will reply the message. So do not reply the
	    message yourself!

	RT_Underscore - (char) [V38] Indicates the symbol that precedes the
	    character in the gadget label to be underscored. This is to
	    define a keyboard shortcut for this gadget. Example: to define
	    the key 'Q' as a keyboard shortcut for "Quit" and 'N' for "Oh,
	    No!" you would use the tag RT_Underscore, '_' and pass as
	    gadfmt "_Quit|Oh, _No!". Do not use the symbol '%' as it is
	    used for string formatting. The usual character to use is '_'
	    like in the example.
	    
	    IMPORTANT: the shortcuts defined using RT_Underscore take
	        precedence of the default shortcuts! It is for example not
	        wise to use a 'N' for a positive response! Pick your
	        shortcuts carefully!

	RT_TextAttr - (struct TextAttr *) [V38] Use this font for the
	    requester. Default is to use the screen font. Note that the
	    font must already be opened by you. ReqTools will call
	    OpenFont() on this TextAttr, _not_ OpenDiskFont()!  If the font
	    cannot be opened using OpenFont() the default screen font will
	    be used.

	RTEZ_ReqTitle - (char *) Title of requester window, default is
	    "Request" unless the requester has less than 2 responses, then
	    the default title is "Information".

	RTEZ_Flags - (ULONG) Flags for rtEZRequestA():

	    EZREQF_NORETURNKEY - turn off the RETURN key as shortcut for
	        positive response.

	    EZREQF_LAMIGAQUAL - keyboard shortcuts are limited to Left
	        Amiga 'V' and 'B', ESC and RETURN.

	    EZREQF_CENTERTEXT - centers each line of body text in the
	        requester window. Useful for about requesters.

	RTEZ_DefaultResponse - (ULONG) Response value that will be returned
	    when the user presses the return key. Will be ignored if the
	    EZREQF_NORETURNKEY flag is set. The text for this response will
	    be printed in bold. Default is 1.

    RESULT
	ret - 1 (TRUE) for leftmost (positive) response, then each
	    consecutive response will return 1 more, the rightmost (false)
	    response will return 0 (FALSE), so 1,2,3,...,num-1,0 -- or
	    idcmp flag.

    NOTES
	Automatically adjusts the requester to the screen font.

	rtEZRequestA() checks the pr_WindowPtr of your process to find the
	screen to put the requester on.

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	exec.library/RawDoFmt(), rtReqHandlerA()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return GetString(bodyfmt,
             (SIPTR)argarray,
             gadfmt,
             0,
             NULL,
             IS_EZREQUEST,
             reqinfo,
             taglist);

    AROS_LIBFUNC_EXIT

} /* rtEZRequestA */
