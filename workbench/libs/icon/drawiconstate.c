/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <intuition/imageclass.h>
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH7(void, DrawIconStateA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct DiskObject *, icon, A1),
	AROS_LHA(STRPTR, label, A2),
	AROS_LHA(LONG, leftEdge, D0),
	AROS_LHA(LONG, topEdge, D1),
	AROS_LHA(ULONG, state, D2),
	AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
	struct Library *, IconBase, 27, Icon)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddFreeList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    
    if (state == IDS_SELECTED && icon->do_Gadget.SelectRender)
    {
	DrawImage(rp,(struct Image*)icon->do_Gadget.SelectRender,leftEdge,topEdge);
    } else
    {
	if (icon->do_Gadget.Flags & GFLG_GADGIMAGE)
	{
	    DrawImage(rp,(struct Image*)icon->do_Gadget.GadgetRender,leftEdge,topEdge);
	}
    }

#warning DrawIconStateA() is only very limited implemented

    AROS_LIBFUNC_EXIT
} /* FreeFreeList */
