/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <proto/intuition.h>

	AROS_LH2(struct Screen *, OpenScreenTagList,

/*  SYNOPSIS */
	AROS_LHA(struct NewScreen *, newScreen, A0),
	AROS_LHA(struct TagItem   *, tagList, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 102, Intuition)

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

    struct ExtNewScreen ns =
    {
	0, 0, 640, 200, 1, 		/* left, top, width, height, depth */
	0, 1, 				/* DetailPen, BlockPen */
	HIRES | LACE, 			/* ViewModes */
	CUSTOMSCREEN | SHOWTITLE, 	/* Type */
	NULL, 				/* Font */
	NULL, 				/* DefaultTitle */
	NULL, 				/* Gadgets */
	NULL, 				/* CustomBitMap */
	NULL 				/* Extension (taglist) */
    };

    if (newScreen)
	CopyMem (newScreen, &ns, (newScreen->Type & NS_EXTENDED) ? sizeof (struct ExtNewScreen) :
								   sizeof (struct NewScreen));

    if (tagList)
    {
    	ns.Extension = tagList;
    	ns.Type |= NS_EXTENDED;
    }
    
    return OpenScreen ((struct NewScreen *)&ns);
    
    AROS_LIBFUNC_EXIT
    
} /* OpenScreenTagList */
