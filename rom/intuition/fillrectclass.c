/*
    (C) 1997-2001 AROS - The Amiga Research OS
    $Id$

    Desc: AROS fillrectclass implementation 
    Lang: english

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

#ifdef _AROS
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "intuition_intern.h"
#include "maybe_boopsi.h"
#endif

#ifdef IntuitionBase
#    undef IntuitionBase
#endif

#define IntuitionBase	((struct IntuitionBase *)(cl->cl_UserData))

/****************************************************************************/

struct FillRectData
{
    WORD apatsize;
    WORD mode;
};

/****************************************************************************/

IPTR fillrect_set(Class *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tag, *tstate = msg->ops_AttrList;
    struct FillRectData *data = INST_DATA(cl, obj);
    
    IPTR retval = 0;
    
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
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

IPTR fillrect_draw(Class *cl, Object *obj, struct impDraw *msg)
{
    struct FillRectData *data = INST_DATA(cl, obj);
    struct RastPort 	*rp = ((struct impDraw *)msg)->imp_RPort;
    WORD 		x1, y1, x2, y2;
    
    APTR		old_areaptrn = rp->AreaPtrn;
    BYTE		old_areaptsz = rp->AreaPtSz;
    BYTE		old_drmd = GetDrMd(rp);
    BYTE		old_apen = GetAPen(rp);
    BYTE		old_bpen = GetBPen(rp);
    	    
    SetABPenDrMd(rp, IM_FGPEN((struct Image *)obj),
    		     IM_BGPEN((struct Image *)obj),
		     data->mode);
	
    SetAfPt(rp, (APTR)((struct Image *)obj)->ImageData, data->apatsize);

    x1 = ((struct Image *)obj)->LeftEdge + ((struct impDraw *)msg)->imp_Offset.X;
    y1 = ((struct Image *)obj)->TopEdge  + ((struct impDraw *)msg)->imp_Offset.Y;
    
    if (msg->MethodID == IM_DRAW)
    {
	x2 = x1 + ((struct Image *)obj)->Width  - 1;
	y2 = y1 + ((struct Image *)obj)->Height - 1;
    } else {
        x2 = x1 + msg->imp_Dimensions.Width  - 1;
	y2 = y1 + msg->imp_Dimensions.Height - 1;
    }

    RectFill(rp, x1, y1, x2, y2);
    
    SetAfPt(rp, old_areaptrn, old_areaptsz);
    SetABPenDrMd(rp, old_apen, old_bpen, old_drmd);
    
    return 0;
}

/****************************************************************************/

AROS_UFH3S(IPTR, dispatch_fillrectclass,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
        case OM_NEW:
    	    obj = (Object *)DoSuperMethodA(cl, obj, msg);
	    if (obj)
	    {
		fillrect_set(cl, obj, (struct opSet *)msg);
		retval = (IPTR)obj;
	    }
	    break;

	case OM_SET:
	    retval = fillrect_set(cl, obj, (struct opSet *)msg);
	    retval += DoSuperMethodA(cl, obj, msg);
	    break;
	
	       
	case IM_DRAW:
	case IM_DRAWFRAME:
	    retval = fillrect_draw(cl, obj, (struct impDraw *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}

/****************************************************************************/

#undef IntuitionBase

/****************************************************************************/

struct IClass *InitFillRectClass (struct IntuitionBase * IntuitionBase)
{
    struct IClass *cl;

    if ( (cl = MakeClass(FILLRECTCLASS, IMAGECLASS, NULL, sizeof(struct FillRectData), 0)) )
    {
	cl->cl_Dispatcher.h_Entry    = (APTR)AROS_ASMSYMNAME(dispatch_fillrectclass);
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)IntuitionBase;

	AddClass (cl);
    }

    return (cl);
}

