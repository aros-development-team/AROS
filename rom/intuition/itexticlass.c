/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

/* Some handy transparent base class object casting defines.
 */
#define G(o)  ((struct Gadget *)o)
#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

LONG itext_draw(Class *cl, Object *o, struct impDraw *msg)
{
    struct RastPort *rp = msg->imp_RPort;
    LONG    	     retval = 0;

    if (rp)
    {
        struct IntuiText    *iText = (struct IntuiText *)IM(o)->ImageData;
        int 	    	     leftOffset = msg->imp_Offset.X + IM(o)->LeftEdge;
        int 	    	     topOffset = msg->imp_Offset.Y + IM(o)->TopEdge;
 
        SetABPenDrMd(rp, IM(o)->PlanePick, 0 ,JAM1);

    	int_PrintIText(rp, iText, leftOffset, topOffset, TRUE, IntuitionBase);
	
        retval = 1;
    }

    return retval;
}

/****************************************************************************/

AROS_UFH3S(IPTR, dispatch_itexticlass,
           AROS_UFHA(Class *,  cl,  A0),
           AROS_UFHA(Object *, o,   A2),
           AROS_UFHA(Msg,      msg, A1)
          )
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch(msg->MethodID)
    {
	case IM_DRAW:
            retval = itext_draw(cl, o, (struct impDraw *)msg);
            break;

    #warning itexticlass/IM_DRAWFRAME not implemented
    #if 0
	case IM_DRAWFRAME:
            itext_drawframe(cl, o, (struct impDraw *)msg);
            break;
    #endif

	default:
            retval = DoSuperMethodA(cl, o, msg);
            break;

    } /* switch */

    return retval;

    AROS_USERFUNC_EXIT
}  /* dispatch_itexticlass */


#undef IntuitionBase

/****************************************************************************/

struct IClass *InitITextIClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl = NULL;

    /* This is the code to make the itexticlass...
    */
    if ( (cl = MakeClass(ITEXTICLASS, IMAGECLASS, NULL, 0, 0)) )
    {
        cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_itexticlass);
        cl->cl_Dispatcher.h_SubEntry = NULL;
        cl->cl_UserData              = (IPTR)IntuitionBase;

        AddClass (cl);
    }

    return (cl);
}

/****************************************************************************/
