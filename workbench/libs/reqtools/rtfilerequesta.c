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

    AROS_LH4(APTR, rtFileRequestA,

/*  SYNOPSIS */

	AROS_LHA(struct rtFileRequester *, filereq, A1),
	AROS_LHA(char *, file, A2),
	AROS_LHA(char *, title, A3),
	AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 9, ReqTools)

/*  FUNCTION
	Get a directory and filename(s), or just a directory from the user.

	'filename' should point to an array of at least 108 chars. The
	filename already in 'filename' will be displayed in the requester
	when it comes up. When the requester returns 'filename' will
	probably have changed.

	Using certain tags may result in the calling of a caller-supplied
	hook.

	The hook will be called with A0 holding the address of your hook
	structure (you may use the h_Data field to your own liking), A2 a
	pointer to the requester structure calling the hook ('req') and A1
	a pointer to an object. The object is variable and depends on what
	your hook is for.

	This is an example of a hook suitable to be used with the
	RTFI_FilterFunc tag:

	SAS/C users can define their function thus:

	BOOL __asm __saveds filterfunc(
	        register __a0 struct Hook *filterhook,
	        register __a2 struct rtFileRequester *req,
	        register __a1 struct FileInfoBlock *fib )
	{
	    BOOL accepted = TRUE;

	    // examine fib to decide if you want this file in the requester 
	    ...
	    return( accepted );
	}

	Your hook structure should then be initialized like this:

	    filterhook->h_Entry = filterfunc;
	    // in this case no need to initialize hook->h_SubEntry
	    filterhook->h_Data = your_userdata_if_needed;

	You can also use a stub written in machine code to call your
	function. (see 'utility/hooks.h')
   
    INPUTS
	filereq  - pointer to a struct rtFileRequester allocated with
	    rtAllocRequestA().
	filename - pointer to an array of chars (must be 108 bytes big).
	title    - pointer to requester window title (null terminated).
	taglist  - pointer to a TagItem array.

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
	        A2 - (struct rtFileRequester *) your requester
	        A1 - (struct IntuiMessage *) the message

	    After you have finished examining the message and your hook
	    returns, ReqTools will reply the message. So do not reply the
	    message yourself!

	RT_Underscore - (char) [V38] Indicates the symbol that precedes the
	    character in a gadget's label to be underscored. This will also
	    define the keyboard shortcut for this gadget. Currently only
	    needed for RTFI_OkText. Usually set to '_'.

	RT_DefaultFont - (struct TextFont *) This tag allows you to specify
	    the font to be used in the requester when the screen font is
	    proportional. Default is GfxBase->DefaultFont. This tag is
	    obsolete in ReqTools 2.2 and higher.

	RT_TextAttr - (struct TextAttr *) [V38] Use this font for the
	    requester. Default is to use the screen font. Note that the
	    font must already be opened by you. ReqTools will call
	    OpenFont() on this TextAttr, _not_ OpenDiskFont()! If the font
	    cannot be opened using OpenFont() or if the font is
	    proportional the default screen font will be used (or the font
	    set with RT_DefaultFont).

	RTFI_Flags - (ULONG) Several flags:

	    FREQF_NOBUFFER - do _not_ use a buffer to remember directory
	        contents for the next time the file requester is used.

	    FREQF_MULTISELECT - allow multiple files to be selected.
	        rtFileRequest() will return a pointer to an rtFileList
	        structure which will contain all selected files. Use
	        rtFreeFileList() to free the memory used by this file list.

	    FREQF_SELECTDIRS - set this flag if you wish to enable the
	        selecting of dirs as well as files. You *must* also set
	        FREQF_MULTISELECT. Directories will be returned together
	        with files in rtFileList, but with StrLen equal to -1. If
	        you need the length of the directory's name use strlen().

	    FREQF_SAVE - Set this if you are using the requester to save or
	        delete something. Double-clicking will be disabled so it is
	        harder to make a mistake and select the wrong file. If the
	        user enters a non-existent directory in the drawer string
	        gadget, a requester will appear asking if the directory
	        should be created.

	    FREQF_NOFILES - Set this if you want to use the requester to
	        allow the user to select a directory rather than a file.
	        Ideal for getting a destination dir. May be used with
	        FREQF_MULTISELECT and FREQF_SELECTDIRS.

	    FREQF_PATGAD - When this is set a pattern gadget will be added
	        to the requester.

	RTFI_Height - (ULONG) Suggested height of file requester window.

	RTFI_OkText - (char *) Replacement text for "Ok" gadget, max 6
	    chars long.

	RTFI_VolumeRequest - (ULONG) [V38] The presence of this tag turns
	    the file requester into a volume/assign disk requester. This
	    requester can be used to get a device name ("DF0:", "DH1:",..)
	    or an assign ("C:", "FONTS:",...) from the user. The result of
	    this requester can be found in the filereq->Dir field. The
	    volume can also be changed with rtChangeReqAttrA() and the
	    RTFI_Dir tag. 

	    Note that the user may edit the disk/assign, or enter a new
	    one. Note also that the real device name is returned, not the
	    name of the volume in the device. For example "DH1:", not
	    "Hard1:". The tag data (ULONG) is used to set following flags:

	    VREQF_NOASSIGNS - Do not include the assigns in the list, only
	        the real devices.

	    VREQF_NODISKS - Do not include devices, just show the assigns.

	    VREQF_ALLDISKS - Show _all_ devices. Default behavior is to
	        show only those devices which have valid disks inserted
	        into them. So if you have no disk in drive DF0: it will not
	        show up. Set this flag if you do want these devices
	        included.

	    NOTE: Do *NOT* use { RTFI_VolumeRequest, TRUE }! You are then
	        setting the VREQF_NOASSIGNS flag! Use { RTFI_VolumeRequest,
	        0 } for a normal volume requester.

	    NOTE: If you use the RTFI_FilterFunc described below the third
	        parameter will be a pointer to a rtVolumeEntry structure
	        rather than a pointer to a FileInfoBlock structure! Tech
	        note: the DOS device list has been unlocked, so it is safe
	        to e.g. Lock() this device and call Info() on this lock.

	    NOTE: A file requester structure allocated with
	        rtAllocRequest() should not be used for both a file and a
	        volume requester. Allocate two requester structures if you
	        need both a file and a volume requester in your program!

	RTFI_FilterFunc - (struct Hook *) [V38] Call this hook for each
	    file and directory in the directory being read (or for each
	    entry in the volume requester). Parameters are as follows:

	    A0 - (struct Hook *) your hook
	    A2 - (struct rtFileRequester *) your filereq
	    A1 - (struct FileInfoBlock *) fib of file OR (struct
	        rtVolumeEntry *) device or assign in case of a volume
	        requester.

	    If your hook returns TRUE the file will be accepted. If it
	    returns FALSE the file will be skipped and will not appear in
	    the requester.

	    IMPORTANT NOTE: If you change your hook's behavior you _MUST_
	        purge the requester's buffer (using rtFreeReqBuffer())!

	    IMPORTANT NOTE: When this callback hook is called from a volume
	        requester the pr_WindowPtr of your process will be set to
	        -1 so *no* DOS requesters will appear when an error occurs!

	RTFI_AllowEmpty - (BOOL) [V38] If RTFI_AllowEmpty is TRUE an empty
	    file string will also be accepted and returned. Defaults to
	    FALSE, meaning that if the user enters no filename the
	    requester will be canceled. You should use this tag as little
	    as possible!

    RESULT
	ret - TRUE if the user selected a file (check 'filereq->Dir' for
	    the directory and 'filename' for the filename) or FALSE if the
	    requester was canceled -- or a pointer to a struct rtFileList
	    (if FREQF_MULTISELECT was used).

    NOTES
	You CANNOT call the file requester from a task because it uses DOS
	calls!

	Automatically adjusts the requester to the screen font.

	If the requester got too big for the screen because of a very large
	font, the topaz.font will be used.

	rtFileRequest() checks the pr_WindowPtr of your process to find the
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

    return FileRequestA((struct RealFileRequester *)filereq, file, title, taglist); /* in filereq.c */
    
    AROS_LIBFUNC_EXIT
    
} /* rtFileRequestA */
