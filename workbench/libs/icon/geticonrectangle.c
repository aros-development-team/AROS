/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH5(BOOL, GetIconRectangleA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct DiskObject *, icon, A1),
	AROS_LHA(STRPTR, label, A2),
	AROS_LHA(struct Rectangle *, rectangle, A3),
	AROS_LHA(struct TagItem *, tags, A4),
/*  LOCATION */
	struct Library *, IconBase, 28, Icon)

/*  FUNCTION
	Query size of icon.
	
    INPUTS
	rp        - reference RastPort (for font)
	icon      - icon to be queried
	label     - label string
	rectangle - resulting size

    RESULT
	TRUE success

    NOTES
	Only very limited implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NativeIcon *nativeicon;

    nativeicon = GetNativeIcon(icon, LB(IconBase));
    if (nativeicon && nativeicon->icon35.img1.imagedata)
    {
	rectangle->MinX = 0;
	rectangle->MinY = 0;
	rectangle->MaxX = nativeicon->icon35.width - 1;
	rectangle->MaxY = nativeicon->icon35.height - 1;
    } else
    {
	rectangle->MinX = 0;
	rectangle->MinY = 0;
	rectangle->MaxX = icon->do_Gadget.Width - 1;
	rectangle->MaxY = icon->do_Gadget.Height - 1;
    }

#warning GetIconRectangleA() is only very limited implemented
    
    return TRUE;
    
    AROS_LIBFUNC_EXIT
} /* GetIconRectangle */
