/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH5(void, GetIconRectangleA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct DiskObject *, icon, A1),
	AROS_LHA(STRPTR, label, A2),
	AROS_LHA(struct Rectangle *, rectangle, A3),
	AROS_LHA(struct TagItem *, tags, A4),
/*  LOCATION */
	struct Library *, IconBase, 28, Icon)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IconBase)
    
    rectangle->MinX = 0;
    rectangle->MinY = 0;
    rectangle->MaxX = icon->do_Gadget.Width - 1;
    rectangle->MaxY = icon->do_Gadget.Height - 1;

#warning GetIconRectangleA() is only very limited implemented

    AROS_LIBFUNC_EXIT
} /* GetIconRectangle */
