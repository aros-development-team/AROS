/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Dtpic.mui. Source based on the one from MUIUndoc */

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>

#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/datatypes.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "dtpic_private.h"

extern struct Library *MUIMasterBase;

#ifdef DataTypesBase
#undef DataTypesBase
#endif

#define DataTypesBase data->datatypesbase

static void killdto(struct Dtpic_DATA *data)
{
    data->bm = NULL;
    data->bmhd = NULL;
    
    if (data->dto)
    {
    	DisposeDTObject(data->dto);
	data->dto = NULL;
    }
    
    if (data->datatypesbase)
    {
    	CloseLibrary(data->datatypesbase);
    }
    
};

IPTR Dtpic__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Dtpic_DATA *data;
 
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
    	data = INST_DATA(cl, obj);
	
	data->name = (STRPTR)GetTagData(MUIA_Dtpic_Name, 0, msg->ops_AttrList);
	
	set(obj, MUIA_FillArea, FALSE);
    }
    
    return (IPTR)obj;
}

IPTR Dtpic__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;
    
    if (data->name)
    {
    	if ((data->datatypesbase = OpenLibrary("datatypes.library", 39)))
	{
	    /* Prevent DOS Requesters from showing up */
	    
	    struct Process *me = (struct Process *)FindTask(0);
	    APTR    	    oldwinptr = me->pr_WindowPtr;
	    
	    me->pr_WindowPtr = (APTR)-1;
	    
	    data->dto = NewDTObject(data->name, DTA_GroupID, GID_PICTURE,
	    	    	    	    	    	OBP_Precision, PRECISION_IMAGE,
						PDTA_Screen, _screen(obj),
						PDTA_FreeSourceBitMap, TRUE,
						PDTA_DestMode, PMODE_V43,
						PDTA_UseFriendBitMap, TRUE,
						TAG_DONE);
    	    me->pr_WindowPtr = oldwinptr;
	    
	    if (data->dto)
	    {
	    	struct FrameInfo fri = {0};
		
		DoMethod(data->dto, DTM_FRAMEBOX, 0, &fri, &fri, sizeof(struct FrameInfo), 0);
		
		if (fri.fri_Dimensions.Depth > 0)
		{
		    if (DoMethod(data->dto, DTM_PROCLAYOUT, 0, 1))
		    {
		    	get(data->dto, PDTA_BitMapHeader, &data->bmhd);
			
			if (data->bmhd)
			{
			    GetDTAttrs(data->dto, PDTA_DestBitMap, &data->bm, TAG_DONE);
			    
			    if (!data->bm)
			    {			    
			    	GetDTAttrs(data->dto, PDTA_BitMap, &data->bm, TAG_DONE);
			    }
			    
			    if (data->bm) return TRUE;
			    
			} /* if (data->bmhd) */
			
		    } /* if (DoMethod(data->dto, DTM_PROCLAYOUT, 0, 1)) */
		    
		} /* if (fri.fri_Dimensions.Depth > 0) */
		
	    } /* if (data->dto) */
	    
	} /* if ((data->datatypesbase = OpenLibrary("datatypes.library", 39))) */
	
    } /* if (data->name) */
    
    killdto(data);
    
    return TRUE;
}

IPTR Dtpic__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    killdto(data);
    
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

IPTR Dtpic__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);
    IPTR    	       retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    
    if (data->bm)
    {
    	msg->MinMaxInfo->MinWidth  += data->bmhd->bmh_Width;
    	msg->MinMaxInfo->MinHeight += data->bmhd->bmh_Height;
    	msg->MinMaxInfo->DefWidth  += data->bmhd->bmh_Width;
    	msg->MinMaxInfo->DefHeight += data->bmhd->bmh_Height;
    	msg->MinMaxInfo->MaxWidth  += data->bmhd->bmh_Width;
    	msg->MinMaxInfo->MaxHeight += data->bmhd->bmh_Height;	
    }
    
    return retval;
}

IPTR Dtpic__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);
    
    if ((msg->flags & MADF_DRAWOBJECT) && data->bm)
    {
    	BltBitMapRastPort(data->bm, 0, 0, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 192);
    }
    
    return 0;
}

#if ZUNE_BUILTIN_DTPIC
BOOPSI_DISPATCHER(IPTR, Dtpic_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:         return Dtpic__OM_NEW(cl, obj, (struct opSet *)msg);
	case MUIM_Setup:     return Dtpic__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup:   return Dtpic__MUIM_Cleanup(cl, obj, (struct MUIP_Clean *)msg);
	case MUIM_AskMinMax: return Dtpic__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
	case MUIM_Draw:      return Dtpic__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
	default:             return DoSuperMethodA(cl, obj, msg);
    }   
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Dtpic_desc =
{ 
    MUIC_Dtpic, 
    MUIC_Area,
    sizeof(struct Dtpic_DATA), 
    (void*)Dtpic_Dispatcher 
};
#endif /* ZUNE_BUILTIN_DTPIC */
