/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>

/*****************************************************************************

    NAME */
	#include <proto/intuition.h>

	AROS_LH2(struct Window *, OpenWindowTagList,

/*  SYNOPSIS */
	AROS_LHA(struct NewWindow *, newWindow, A0),
	AROS_LHA(struct TagItem   *, tagList, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 101, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

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

    struct ExtNewWindow nw =
    {
	0, 0, 		/* Left, Top */
	~0, ~0, 	/* Width, Height */
	0xFF, 0xFF, 	/* DetailPen, BlockPen */
	0L, 		/* IDCMPFlags */
	0L, 		/* Flags */
	NULL, 		/* FirstGadget */
	NULL, 		/* CheckMark */
	NULL, 		/* Title */
	NULL, 		/* Screen */
	NULL, 		/* BitMap */
	0, 0, 		/* MinWidth, MinHeight */
	0, 0, 		/* MaxWidth, MaxHeight */
	WBENCHSCREEN, 	/* Type */
	NULL 		/* Extension (taglist) */
    };
    struct Window       *window;

    if (newWindow)
    {
	ASSERT_VALID_PTR(newWindow);
	CopyMem (newWindow, &nw, (newWindow->Flags & WFLG_NW_EXTENDED) ? sizeof (struct ExtNewWindow) :
									 sizeof (struct NewWindow));
    }

    if (tagList)
    {
	/* valid taglists allocated in code segment lead to spurious assertion failures
	 * ASSERT_VALID_PTR(tagList);
	 */
    	nw.Extension = tagList;
    	nw.Flags |= WFLG_NW_EXTENDED;
    }
    
    window = OpenWindow ((struct NewWindow *)&nw);

    return window;
    
    AROS_LIBFUNC_EXIT
    
} /* OpenWindowTagList */
