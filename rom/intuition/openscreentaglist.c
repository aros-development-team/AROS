/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  2000/08/03 20:36:32  stegerg
    screen depth gadget should be usable now + src cleanup + small fixes

    Revision 1.8  2000/01/21 23:04:52  stegerg
    SHOWTITLE (SA_ShowTitle) defaults to TRUE:

    Revision 1.7  1999/08/16 21:07:33  stegerg
    taglist is now handled in openscreen

    Revision 1.6  1998/10/20 16:46:01  hkiel
    Amiga Research OS

    Revision 1.5  1998/09/12 20:20:08  hkiel
    converted TODO/FIXME comments to #warnings

    Revision 1.4  1997/01/27 00:36:41  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:06  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 14:11:39  digulla
    Open and close screens


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
