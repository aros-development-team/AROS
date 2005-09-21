/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    AROS fillrectclass implementation.
*/

#include <exec/types.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
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
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#endif /* !__MORPHOS__ */

#ifdef IntuitionBase
#    undef IntuitionBase
#endif

#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

IPTR fillrect_set(Class *cl, Object *obj, struct opSet *msg)
{
    struct TagItem  	*tag, *tstate = msg->ops_AttrList;
    struct FillRectData *data = INST_DATA(cl, obj);

    IPTR retval = 0;

    while((tag = NextTagItem(&tstate)))
    {
        switch(tag->ti_Tag)
        {
            case IA_APattern:
        	((struct Image *)obj)->ImageData = (APTR)tag->ti_Data;
        	retval = 1;
        	break;

            case IA_APatSize:
        	data->apatsize = (WORD)tag->ti_Data;
        	retval = 1;
        	break;

            case IA_Mode:
        	data->mode = (WORD)tag->ti_Data;
        	retval = 1;
        	break;
        }
    }

    return retval;
}

/****************************************************************************/

IPTR FillRectClass__IM_DRAW(Class *cl, Object *obj, struct impDraw *msg)
{
    struct FillRectData *data = INST_DATA(cl, obj);
    struct RastPort      rp;
    WORD            	 x1, y1, x2, y2;

    if (!((struct impDraw *)msg)->imp_RPort) return 0;

    memcpy(&rp,((struct impDraw *)msg)->imp_RPort,sizeof (struct RastPort));
    
    SetABPenDrMd(&rp, IM_FGPEN((struct Image *)obj),
                 IM_BGPEN((struct Image *)obj),
                 data->mode);

    SetAfPt(&rp, (APTR)((struct Image *)obj)->ImageData, data->apatsize);

    x1 = ((struct Image *)obj)->LeftEdge + ((struct impDraw *)msg)->imp_Offset.X;
    y1 = ((struct Image *)obj)->TopEdge  + ((struct impDraw *)msg)->imp_Offset.Y;

    if (msg->MethodID == IM_DRAW)
    {
        x2 = x1 + ((struct Image *)obj)->Width  - 1;
        y2 = y1 + ((struct Image *)obj)->Height - 1;
    }
    else
    {
        x2 = x1 + msg->imp_Dimensions.Width  - 1;
        y2 = y1 + msg->imp_Dimensions.Height - 1;
    }

    RectFill(&rp, x1, y1, x2, y2);

    return 0;
}

/****************************************************************************/

IPTR FillRectClass__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
	fillrect_set(cl, obj, msg);

    return (IPTR)obj;
}

/****************************************************************************/

IPTR FillRectClass__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    return fillrect_set(cl, obj, msg) + DoSuperMethodA(cl, obj, (Msg)msg);
}

/****************************************************************************/
