/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    AROS menubarlabelclass implementation (does not exist in AmigaOS)
*/

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/rpattr.h>

#include <cybergraphx/cybergraphics.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>

#ifndef __MORPHOS__
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#endif /* !__MORPHOS__ */

#ifdef SKINS
#   include "intuition_customize.h"
#   include "intuition_extend.h"
#endif

#ifdef IntuitionBase
#    undef IntuitionBase
#endif

#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

/*************************** BarLabelClass *****************************/

IPTR MenuBarLabelClass__OM_NEW(Class *cl, Object *obj, Msg msg)
{
    struct Image *im = (struct Image *)DoSuperMethodA(cl, obj, msg);
    
    if (im)
    {
	struct MenuBarLabelData *data = INST_DATA(cl, im);
	data->dri = NULL;
    }
    
    return (IPTR)im;
}

/****************************************************************************/

IPTR MenuBarLabelClass__OM_SET(Class *cl, struct Image *im, struct opSet *msg)
{
    struct TagItem *ti;
    
    if ((ti = FindTagItem(SYSIA_DrawInfo, msg->ops_AttrList)))
    {
	struct MenuBarLabelData *data = INST_DATA(cl, im);

	data->dri = (struct DrawInfo *)ti->ti_Data;
    }
    
    return DoSuperMethodA(cl, (Object *)im, (Msg)msg);
}

/****************************************************************************/

IPTR MenuBarLabelClass__OM_GET(Class *cl, struct Image *im, struct opGet *msg)
{
    switch(msg->opg_AttrID)
    {
    case IA_SupportsDisable:
	if (MENUS_AMIGALOOK)
	{
	    *(msg->opg_Storage) = 0;
	}
	else
	{
	    *(msg->opg_Storage) = 1;
	}
	return (IPTR)1;
	
    default:
	return DoSuperMethodA(cl, (Object *)im, (Msg)msg);
    }
}

/****************************************************************************/

IPTR MenuBarLabelClass__IM_DRAW(Class *cl, struct Image *im, struct impDraw *msg)
{
    struct MenuBarLabelData *data = INST_DATA(cl, im);

    if (data->dri)
    {
	struct RastPort *rp = msg->imp_RPort;
	WORD x1, y1, x2, y2;
	
	if (!rp) return (IPTR)0;

	SetDrMd(rp, JAM1);

	x1 = im->LeftEdge + msg->imp_Offset.X;
	y1 = im->TopEdge  + msg->imp_Offset.Y;
	x2 = x1 + im->Width  - 1;
	y2 = y1 + im->Height - 1;

	
	if (MENUS_AMIGALOOK)
	{
	    SetAPen(rp, data->dri->dri_Pens[BARDETAILPEN]);
	    RectFill(rp, x1, y1, x2, y2);
	}
	else
	{
	    /* Will only work if imageheight = 2 */
	    SetAPen(rp, data->dri->dri_Pens[SHADOWPEN]);
	    RectFill(rp, x1, y1, x2 - 1, y1);
	    WritePixel(rp, x1, y2);

	    SetAPen(rp, data->dri->dri_Pens[SHINEPEN]);
	    RectFill(rp, x1 + 1, y2, x2, y2);
	    WritePixel(rp, x2, y1);
	}
		
    } /* if (data->dri) */
    
    return (IPTR)0;
}

/****************************************************************************/
