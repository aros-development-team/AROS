/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition private function SetDefaultScreenFont()
    Lang: English
*/
#include "intuition_intern.h"

/*****************************************************************************

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

    NOTES
    	PRIVATE(!!!!) Do not use
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (textfont)
    {
    	ASSERT_VALID_PTR(textfont);
	
	Forbid();
	GetPrivIBase(IntuitionBase)->ScreenFont = textfont;
	Permit();
    }
      
    AROS_LIBFUNC_EXIT
    
} /* SetDefaultScreenFont */
