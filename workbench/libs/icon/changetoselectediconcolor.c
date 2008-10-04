/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "icon_intern.h"

#define EFFECT_NONE      (0)
#define EFFECT_LIGHTEN   (1)
#define EFFECT_TINT_BLUE (2)
#define EFFECT_XOR       (3)

#define EFFECT EFFECT_LIGHTEN

/*****************************************************************************

    NAME */

    AROS_LH1(VOID, ChangeToSelectedIconColor,

/*  SYNOPSIS */
        AROS_LHA(struct ColorRegister *, cr, A0),

/*  LOCATION */
        struct Library *, IconBase, 33, Icon)

/*  FUNCTION
	Change a color register for selected icon state.
	
    INPUTS
	cr - colorregister to be changed.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
#if EFFECT == EFFECT_LIGHTEN
    cr->red   = (cr->red   >> 1) + (0xFF >> 1);
    cr->green = (cr->green >> 1) + (0xFF >> 1);
    cr->blue  = (cr->blue  >> 1) + (0xFF >> 1);
#elif EFFECT == EFFECT_TINT_BLUE
    cr->red   = (cr->red   >> 1);
    cr->green = (cr->green >> 1);
    cr->blue  = (cr->blue  >> 1) + (0xFF >> 2);
#elif EFFECT == EFFECT_XOR
    cr->red   = cr->red   ^ 0xFF;
    cr->green = cr->green ^ 0xFF;
    cr->blue  = cr->blue  ^ 0xFF;
#endif
    
    AROS_LIBFUNC_EXIT
} /* ChangeToSelectedIconColor() */
