/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

/****************************************************************************/

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <string.h>

#ifndef __MORPHOS__
#include "intuition_intern.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"
#endif /* !__MORPHOS__ */

/****************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

IPTR ITextIClass__IM_DRAW(Class *cl, struct Image *im, struct impDraw *msg)
{
    struct RastPort *rp = msg->imp_RPort;
    IPTR    	     retval = (IPTR)0;

    if (rp)
    {
        struct IntuiText    *iText = (struct IntuiText *)im->ImageData;
        int 	    	     leftOffset = msg->imp_Offset.X + im->LeftEdge;
        int 	    	     topOffset = msg->imp_Offset.Y + im->TopEdge;
 
        SetABPenDrMd(rp, im->PlanePick, 0 ,JAM1);

    	int_PrintIText(rp, iText, leftOffset, topOffset, TRUE, IntuitionBase);
	
        retval = (IPTR)1;
    }

    return retval;
}

#warning ITextIClass::IM_DRAWFRAME not implemented
