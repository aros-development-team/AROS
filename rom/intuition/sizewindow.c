/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  1999/10/12 21:05:59  stegerg
    noop if dx and dy = 0

    Revision 1.8  1999/03/25 04:26:23  bergers
    Update for deffered treatment of windows.

    Revision 1.7  1998/10/20 16:46:05  hkiel
    Amiga Research OS

    Revision 1.6  1997/01/27 00:36:43  ldp
    Polish

    Revision 1.5  1996/12/10 14:00:10  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/11/08 11:28:04  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.3  1996/10/24 15:51:25  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/08/29 13:57:38  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.1  1996/08/23 17:28:17  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH3(void, SizeWindow,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(LONG           , dx, D0),
	AROS_LHA(LONG           , dy, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 48, Intuition)

/*  FUNCTION
	Modify the size of a window by the specified offsets.

    INPUTS
	window - The window to resize.
	dx - Add this to the width.
	dy - Add this to the height.

    RESULT
	None.

    NOTES
	The resize of the window may be delayed. If you depend on the
	information that is has changed size, wait for IDCMP_NEWSIZE.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct shortIntuiMessage * msg;
 
    if (dx || dy)
    {   
	msg = AllocMem(sizeof(struct shortIntuiMessage), MEMF_CLEAR);

	if (NULL != msg)
	{
	  msg->Class       = IDCMP_WBENCHMESSAGE;
	  msg->Code        = IMCODE_SIZEWINDOW;
	  msg->Window      = window;
	  msg->dx          = dx;
	  msg->dy          = dy;

	  PutMsg(window->WindowPort, (struct Message *)msg); 
	}   
    }

    AROS_LIBFUNC_EXIT
} /* SizeWindow */
