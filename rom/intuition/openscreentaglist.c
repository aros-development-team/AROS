/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
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
        0, 0, -1, -1, 1,            /* left, top, width, height, depth */
        0, 1,               	    /* DetailPen, BlockPen */
        HIRES | LACE,               /* ViewModes */
        CUSTOMSCREEN | SHOWTITLE,   /* Type */
        NULL,               	    /* Font */
        NULL,               	    /* DefaultTitle */
        NULL,               	    /* Gadgets */
        NULL,               	    /* CustomBitMap */
        NULL                	    /* Extension (taglist) */
    };

    DEBUG_OPENSCREENTAGLIST(dprintf("OpenScreenTagList: NewScreen 0x%lx Tags 0x%lx\n",
                                    newScreen, tagList));
    ns.DetailPen = GetPrivIBase(IntuitionBase)->DriPens4[DETAILPEN];
    ns.BlockPen  = GetPrivIBase(IntuitionBase)->DriPens4[BLOCKPEN];

    if (newScreen)
        CopyMem (newScreen, &ns, (newScreen->Type & NS_EXTENDED) ? sizeof (struct ExtNewScreen) :
                 sizeof (struct NewScreen));

    if (tagList)
    {
        ns.Extension = tagList;
        ns.Type |= NS_EXTENDED;
    }

#ifdef __MORPHOS__
    /* calling OpenScreen through the library vector causes a loop with cgx's patch. */
    {
        extern ULONG LIB_OpenScreen(void);
	
        REG_A0 = (LONG)&ns;
        REG_A6 = (LONG)IntuitionBase;
	
        return (struct Screen *) LIB_OpenScreen();
    }
#else
    return OpenScreen ((struct NewScreen *)&ns);
#endif

    AROS_LIBFUNC_EXIT

} /* OpenScreenTagList */
