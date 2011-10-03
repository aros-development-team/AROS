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
	struct IconBase *, IconBase, 28, Icon)

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
    BOOL isFrameless;
    BOOL isBorderless;
    struct Rectangle rect = { 0, 0, 0, 0};

    isFrameless = GetTagData(ICONDRAWA_Frameless, FALSE, tags);
    isBorderless = GetTagData(ICONDRAWA_Borderless, FALSE, tags);

    if (!isBorderless) {
        rect = LB(IconBase)->ib_EmbossRectangle;
        if (!isFrameless) {
            rect.MinX -= 1;
            rect.MinY -= 1;
            rect.MaxX += 1;
            rect.MaxY += 1;
        }
    }

    nativeicon = GetNativeIcon(icon, LB(IconBase));
    if (nativeicon && nativeicon->ni_Screen)
    {
	rectangle->MinX = rect.MinX + 0;
	rectangle->MinY = rect.MinY + 0;
	rectangle->MaxX = rect.MaxX + nativeicon->ni_Width - 1;
	rectangle->MaxY = rect.MaxY + nativeicon->ni_Height - 1;
    } else
    {
	rectangle->MinX = rect.MinX + 0;
	rectangle->MinY = rect.MinY + 0;
	rectangle->MaxX = rect.MaxX + icon->do_Gadget.Width - 1;
	rectangle->MaxY = rect.MaxY + icon->do_Gadget.Height - 1;
    }

    if (label != NULL) {
    	struct TextExtent extent;
        LONG txtlen = strlen(label);

        if (txtlen > IconBase->ib_MaxNameLength)
            txtlen = IconBase->ib_MaxNameLength;

    	TextExtent(rp, label, txtlen, &extent);

    	rectangle->MaxY += extent.te_Height;

    	if (extent.te_Width > (rectangle->MaxX+1))
    	    rectangle->MaxX  = extent.te_Width - 1;
    }

    return TRUE;
    
    AROS_LIBFUNC_EXIT
} /* GetIconRectangle */
