/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****i***********************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(void, SetDefaultScreenFont,

/*  SYNOPSIS */
        AROS_LHA(struct TextFont *, textfont, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 144, Intuition)

/*  FUNCTION
        Set the default Font.

    INPUTS
        textfont - The Font to be used.

    RESULT
        None.

    NOTES
        This function is actually private and intended only for use
        by IPrefs program.
        This private function is also present in MorphOS v50.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (textfont)
    {
        ASSERT_VALID_PTR(textfont);

        Forbid();
        GetPrivIBase(IntuitionBase)->ScreenFont = textfont;
        Permit();
    }

    AROS_LIBFUNC_EXIT

} /* SetDefaultScreenFont */
