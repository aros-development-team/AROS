/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"
#include "support_classes.h"
#include "colorfield_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#define FLAG_FIXED_PEN 	    1
#define FLAG_PEN_ALLOCATED  2
#define FLAG_NO_PEN 	    4


IPTR Colorfield__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Colorfield_DATA   *data;
    struct TagItem  	    	*tag, *tags;
    ULONG   	    	    	*rgb;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Colorfield_Red:
	    	data->rgb[0] = (ULONG)tag->ti_Data;
		break;
		
	    case MUIA_Colorfield_Green:
	    	data->rgb[1] = (ULONG)tag->ti_Data;
		break;
		
	    case MUIA_Colorfield_Blue:
	    	data->rgb[2] = (ULONG)tag->ti_Data;
		break;
		
	    case MUIA_Colorfield_RGB:
	    	rgb = (ULONG *)tag->ti_Data;
		data->rgb[0] = *rgb++;
		data->rgb[1] = *rgb++;
		data->rgb[2] = *rgb++;
		break;
		
	    case MUIA_Colorfield_Pen:
	    	data->pen = (UBYTE)tag->ti_Data;
	    	data->pen = FLAG_FIXED_PEN;
		break;

   	}
    }
 
    return (IPTR)obj;
}

IPTR Colorfield__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Colorfield_DATA   *data;
    struct TagItem  	    	*tag, *tags;
    ULONG   	    	    	*rgb;
    BOOL    	    	    	 newcol = FALSE;
    IPTR    	    	    	 retval;
    
    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Colorfield_Red:
	    	data->rgb[0] = (ULONG)tag->ti_Data;
		newcol = TRUE;
		break;
		
	    case MUIA_Colorfield_Green:
	    	data->rgb[1] = (ULONG)tag->ti_Data;
		newcol = TRUE;
		break;
		
	    case MUIA_Colorfield_Blue:
	    	data->rgb[2] = (ULONG)tag->ti_Data;
		newcol = TRUE;
		break;
		
	    case MUIA_Colorfield_RGB:
	    	rgb = (ULONG *)tag->ti_Data;
		data->rgb[0] = *rgb++;
		data->rgb[1] = *rgb++;
		data->rgb[2] = *rgb++;
		newcol = TRUE;
		break;
		
	    case MUIA_Colorfield_Pen:
	    	if (data->flags & FLAG_PEN_ALLOCATED)
		{
		    ReleasePen(_screen(obj)->ViewPort.ColorMap, data->pen);
		    data->flags &= ~(FLAG_PEN_ALLOCATED | FLAG_NO_PEN);
		}
		data->pen = (UBYTE)data->pen;
		data->flags |= FLAG_FIXED_PEN;
		break;
		
    	}
    }

    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (newcol && (_flags(obj) & MADF_SETUP))
    {
    	SetRGB32(&_screen(obj)->ViewPort, data->pen, data->rgb[0], data->rgb[1], data->rgb[2]);
	
	if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) > 8)
	{
	    MUI_Redraw(obj, MADF_DRAWUPDATE);
	}
    }
    
    return retval;
}

IPTR Colorfield__OM_GET(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct Colorfield_DATA *data  = INST_DATA(cl, obj);
    IPTR    	    	      *store = msg->opg_Storage;

    switch (msg->opg_AttrID)
    {
    	case MUIA_Colorfield_Red:
	    *store = data->rgb[0];
	    break;
	    
	case MUIA_Colorfield_Green:
	    *store = data->rgb[1];
	    break;
	    
	case MUIA_Colorfield_Blue:
	    *store = data->rgb[2];
	    break;
	    
	case MUIA_Colorfield_RGB:
	    *(ULONG **)store = data->rgb;
	    break;
	    
	case MUIA_Colorfield_Pen:
	    *store = data->pen;
	    break;
	    
    	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    return TRUE;
}

IPTR Colorfield__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl,obj);

    if (!(DoSuperMethodA(cl, obj, (Msg)msg))) return 0;

    if (data->flags & FLAG_FIXED_PEN)
    {
    	SetRGB32(&_screen(obj)->ViewPort,
	    	 data->pen,
		 data->rgb[0],
		 data->rgb[1],
		 data->rgb[2]);
    }
    else
    {
    	LONG pen;
	
	pen = ObtainPen(_screen(obj)->ViewPort.ColorMap,
	    	    	(ULONG)-1,
			data->rgb[0],
			data->rgb[1],
			data->rgb[2],
			PENF_EXCLUSIVE);
			
	if (pen == -1)
	{
	    data->flags |= FLAG_NO_PEN;
	    data->pen = 0;
	}
	else
	{
    	    data->pen = (UBYTE)pen;
	    data->flags |= FLAG_PEN_ALLOCATED;
	}
    }
    
    return 1;
}

IPTR Colorfield__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl,obj);

    if (data->flags & FLAG_PEN_ALLOCATED)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->pen);
	data->flags &= ~FLAG_PEN_ALLOCATED;
	data->pen = 0;
    }
    data->flags &= ~FLAG_NO_PEN;
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Colorfield__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl,obj,(Msg)msg);

    msg->MinMaxInfo->MinWidth  += 1;
    msg->MinMaxInfo->MinHeight += 1;
    msg->MinMaxInfo->DefWidth  += 16;
    msg->MinMaxInfo->DefHeight += 16;
    msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
	
    return 0;
}

IPTR Colorfield__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Colorfield_DATA *data = INST_DATA(cl,obj);

    DoSuperMethodA(cl,obj,(Msg)msg);
    
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    if (data->flags & FLAG_NO_PEN)
    {
    	static UWORD pat[] = {0x1111,0x2222,0x4444,0x8888};
	
    	SetAfPt(_rp(obj), pat, 2);
	SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_SHADOW], _pens(obj)[MPEN_BACKGROUND],JAM2);

    }
    else
    {
	SetABPenDrMd(_rp(obj), data->pen, 0,JAM1);
    }
    
    RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
    
    SetAfPt(_rp(obj), NULL, 0);
    
    return 0;
}

#if ZUNE_BUILTIN_COLORFIELD
BOOPSI_DISPATCHER(IPTR, Colorfield_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:         return Colorfield__OM_NEW(cl, obj, (struct opSet *)msg);
	case OM_SET:         return Colorfield__OM_SET(cl, obj, (struct opSet *)msg);
	case OM_GET:         return Colorfield__OM_GET(cl, obj, (struct opGet *)msg);
	case MUIM_Setup:     return Colorfield__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_Cleanup:   return Colorfield__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	case MUIM_AskMinMax: return Colorfield__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
	case MUIM_Draw:      return Colorfield__MUIM_Draw(cl, obj, (struct MUIP_Draw*)msg);
        default:             return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Colorfield_desc =
{ 
    MUIC_Colorfield, 
    MUIC_Area, 
    sizeof(struct Colorfield_DATA), 
    (void*)Colorfield_Dispatcher 
};
#endif /* ZUNE_BUILTIN_COLORFIELD */
