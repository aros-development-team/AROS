/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

struct MUI_PendisplayData
{
    struct MUI_PenSpec  penspec;
    struct MUI_RGBcolor rgb;
    Object  	    	*refobj;
    LONG    	    	pen;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Pendisplay_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PendisplayData   *data;
    struct TagItem  	    	*tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    snprintf(data->penspec.ps_buf, sizeof(data->penspec.ps_buf), "%c%d", PST_MUI, MPEN_TEXT);
    data->pen = -1;
    
    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Pendisplay_Reference:
	    	data->refobj = (Object *)tag->ti_Data;
		DoMethod(data->refobj, MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime,
		    	 (IPTR)obj, 3, MUIM_Set, MUIA_Pendisplay_Spec, MUIV_TriggerValue);
		break;
		
	    case MUIA_Pendisplay_RGBcolor:
	    	{
		    struct MUI_RGBcolor *rgb = (struct MUI_RGBcolor *)tag->ti_Data;
		    
	    	    snprintf(data->penspec.ps_buf, sizeof(data->penspec.ps_buf), "%c%08x,%08x,%08x", PST_RGB, rgb->red, rgb->green, rgb->blue);
		}
		break;
		
	    case MUIA_Pendisplay_Spec:
	    	data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
		break;
		
   	}
    }
 
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Pendisplay_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_PendisplayData   *data;
 
    data = INST_DATA(cl, obj);
    if (data->refobj)
    {
    	/* hmm ... kill notify here? But refobj might already have been killed :-\ */
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Pendisplay_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_PendisplayData   *data;
    struct TagItem  	    	*tag, *tags;
   BOOL    	    	    	 newcol = FALSE;
    IPTR    	    	    	 retval;
    
    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Pendisplay_Reference:
	    	if (data->refobj)
		{
		    DoMethod(data->refobj, MUIM_KillNotifyObj, MUIA_Pendisplay_Spec, (IPTR)obj);
		}

	    	data->refobj = (Object *)tag->ti_Data;
		
		if (data->refobj)
		{
		    DoMethod(data->refobj, MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime,
		    	     (IPTR)obj, 3, MUIM_Set, MUIA_Pendisplay_Spec, MUIV_TriggerValue);
		}
		newcol = TRUE;
		break;

	    case MUIA_Pendisplay_RGBcolor:
	    	{
		    struct MUI_RGBcolor *rgb = (struct MUI_RGBcolor *)tag->ti_Data;
		    
	    	    snprintf(data->penspec.ps_buf, sizeof(data->penspec.ps_buf), "%c%08x,%08x,%08x", PST_RGB, rgb->red, rgb->green, rgb->blue);
		}
		newcol = TRUE;
		break;
		
	    case MUIA_Pendisplay_Spec:
	    	data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
		newcol = TRUE;
		break;

		
    	}
    }

    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (newcol && (_flags(obj) & MADF_SETUP))
    {
    	if (data->pen != -1)
	{
	    MUI_ReleasePen(muiRenderInfo(obj), data->pen);
	    data->pen = -1;
	}
	
	if (!data->refobj)
	{
	    data->pen = MUI_ObtainPen(muiRenderInfo(obj), &data->penspec, 0);
	}
	
	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return retval;
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG  Pendisplay_Get(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct MUI_PendisplayData *data  = INST_DATA(cl, obj);
    IPTR    	    	      *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    	case MUIA_Pendisplay_Pen:
	    *store = (IPTR)data->pen;
	    break;
	    
	case MUIA_Pendisplay_Reference:
	    *store = (IPTR)data->refobj;
	    break;
	    
	case MUIA_Pendisplay_RGBcolor:
	    #warning "FIXME: MUIA_Pendisplay_RGBcolor"
	    *store = (IPTR)&data->rgb;
	    break;
	    
	case MUIA_Pendisplay_Spec:
	    *store = (IPTR)&data->penspec;
	    break;
	    
    	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    return TRUE;
}


/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Pendisplay_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_PendisplayData *data = INST_DATA(cl,obj);

    if (!(DoSuperMethodA(cl, obj, (Msg)msg))) return 0;

    if (!data->refobj)
    {
    	data->pen = MUI_ObtainPen(muiRenderInfo(obj), &data->penspec, 0);
    }
    
    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Pendisplay_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_PendisplayData *data = INST_DATA(cl,obj);

    if (data->pen != -1)
    {
    	MUI_ReleasePen(muiRenderInfo(obj), data->pen);
	data->pen = -1;
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Pendisplay_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth  += 3;
    msg->MinMaxInfo->MinHeight += 3;
    msg->MinMaxInfo->DefWidth  += 16;
    msg->MinMaxInfo->DefHeight += 16;
    msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
	
    return 0;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Pendisplay_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_PendisplayData *data = INST_DATA(cl,obj);
    LONG    	    	       color;
    
    DoSuperMethodA(cl,obj,(Msg)msg);
    
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    if (data->refobj)
    {
    	IPTR val;
	
	get(data->refobj, MUIA_Pendisplay_Pen, &val);	
    	color = (LONG)val;
    }
    else
    {
    	color = data->pen;
    }
    
    if (color == -1)
    {
    	static UWORD pat[] = {0x1111,0x2222,0x4444,0x8888};
	
    	SetAfPt(_rp(obj), pat, 2);
	SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_SHADOW], _pens(obj)[MPEN_BACKGROUND],JAM2);

    }
    else
    {
	SetABPenDrMd(_rp(obj), MUIPEN(data->pen), 0,JAM1);
    }
    
    RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
    
    SetAfPt(_rp(obj), NULL, 0);
    
    return 0;
}


/**************************************************************************
 MUIM_Pendisplay_SetColormap
**************************************************************************/
static IPTR Pendisplay_SetColormap(struct IClass *cl, Object *obj, struct MUIP_Pendisplay_SetColormap *msg)
{
    struct MUI_PenSpec penspec;
    
    snprintf(penspec.ps_buf, sizeof(penspec.ps_buf), "%c%d", PST_CMAP, msg->colormap);    
    set(obj, MUIA_Pendisplay_Spec, (IPTR)&penspec);
   
    return 0;
}

/**************************************************************************
 MUIM_Pendisplay_SetRGB
**************************************************************************/
static IPTR Pendisplay_SetRGB(struct IClass *cl, Object *obj, struct MUIP_Pendisplay_SetRGB *msg)
{
    struct MUI_PenSpec penspec;

    snprintf(penspec.ps_buf, sizeof(penspec.ps_buf), "%c%08x,%08x,%08x", PST_RGB, msg->r, msg->g, msg->b);
    set(obj, MUIA_Pendisplay_Spec, (IPTR)&penspec);
   
    return 0;
}

/**************************************************************************
 MUIM_Pendisplay_SetMUIPen
**************************************************************************/
static IPTR Pendisplay_SetMUIPen(struct IClass *cl, Object *obj, struct MUIP_Pendisplay_SetMUIPen *msg)
{
    struct MUI_PenSpec penspec;
    
    snprintf(penspec.ps_buf, sizeof(penspec.ps_buf), "%c%d", PST_MUI, msg->muipen);
    set(obj, MUIA_Pendisplay_Spec, (IPTR)&penspec);
   
    return 0;
}

BOOPSI_DISPATCHER(IPTR, Pendisplay_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Pendisplay_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Pendisplay_Dispose(cl, obj, msg);
	case OM_SET: return Pendisplay_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Pendisplay_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return Pendisplay_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup: return Pendisplay_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	case MUIM_AskMinMax: return Pendisplay_AskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
	case MUIM_Draw: return Pendisplay_Draw(cl, obj, (struct MUIP_Draw*)msg);
	case MUIM_Pendisplay_SetColormap: return Pendisplay_SetColormap(cl, obj, (struct MUIP_Pendisplay_SetColormap *)msg);
	case MUIM_Pendisplay_SetMUIPen: return Pendisplay_SetMUIPen(cl, obj, (struct MUIP_Pendisplay_SetMUIPen *)msg);
	case MUIM_Pendisplay_SetRGB: return Pendisplay_SetRGB(cl, obj, (struct MUIP_Pendisplay_SetRGB *)msg);
	
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Pendisplay_desc = { 
    MUIC_Pendisplay, 
    MUIC_Area, 
    sizeof(struct MUI_PendisplayData), 
    (void*)Pendisplay_Dispatcher 
};

