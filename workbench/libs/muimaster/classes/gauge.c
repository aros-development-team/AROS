/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <string.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#define GAUGE_BUFSIZE 256

struct MUI_GaugeData
{
   BOOL horiz;
   BOOL dupinfo;

   ULONG current; /* (dlc) LONG in MUI, (sba) but MUI seems to handle it like ULONG */
   ULONG max;
   ULONG divide;
   STRPTR info;

   char buf[GAUGE_BUFSIZE];
   LONG info_width;
   LONG info_height;
};

static char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Gauge_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GaugeData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperNew(cl, obj,
			       MUIA_Background, MUII_BACKGROUND,
			       MUIA_Frame, MUIV_Frame_Gauge,
			       TAG_MORE, msg->ops_AttrList);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    data->divide = 1;
    data->max = 100;
    data->horiz = FALSE;
    data->current = 0;
    data->dupinfo = FALSE;
    data->info = NULL;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Gauge_Current:
	    	    data->current = tag->ti_Data;
		    break;
	    case    MUIA_Gauge_Divide:
	    	    data->divide = tag->ti_Data;
		    break;
	    case    MUIA_Gauge_Horiz:
		    data->horiz = tag->ti_Data;
		    break;
	    case    MUIA_Gauge_InfoText:
	    	    data->info = (STRPTR)tag->ti_Data;
		    break;
	    case    MUIA_Gauge_Max:
	    	    data->max = tag->ti_Data;
		    break;
	    case    MUIA_Gauge_DupInfoText:
		    data->dupinfo = tag->ti_Data;
		    break;
    	}
    }
    if (data->divide != 0)
	data->current /= data->divide;
    if (data->current > data->max)
	data->current = data->max;
    if (data->dupinfo)
	data->info = StrDup(data->info);

    if (data->info)
    {
#ifdef _AROS
	snprintf(data->buf, GAUGE_BUFSIZE, data->info, data->current);
#else
	sprintf(data->buf, data->info, data->current);
#endif
    } else data->buf[0] = 0;

    D(bug("muimaster.library/gauge.c: Gauge Object created at 0x%lx\n",obj));

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Gauge_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_GaugeData   *data = INST_DATA(cl,obj);
    if (data->dupinfo && data->info) FreeVec(data->info);
    return DoSuperMethodA(cl,obj,msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Gauge_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_GaugeData   *data;
    struct TagItem  	    *tag, *tags;
    int info_changed = 0;
    int need_redraw = 0;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Gauge_Current:
		    if (data->current != tag->ti_Data)
		    {
		        data->current = tag->ti_Data;
		        info_changed = 1;
		        need_redraw = 1;
		    }
		    else
		    {
		        tag->ti_Tag = TAG_IGNORE;
		    }
		    break;

	    case    MUIA_Gauge_Divide:
		    if (data->divide != tag->ti_Data)
		    {
		        data->divide = tag->ti_Data;
		        info_changed = 1;
		        need_redraw = 1;
		    }
		    else
		    {
		        tag->ti_Tag = TAG_IGNORE;
		    }
		    break;

		case    MUIA_Gauge_InfoText:
	    	    if (strcmp(data->info, (STRPTR)tag->ti_Data))
		    {
		        if (data->dupinfo)
	    	        {
			    if (data->info) FreeVec(data->info);
			    data->info = StrDup((STRPTR)tag->ti_Data);
		        } else
		        {
			    data->info = (STRPTR)tag->ti_Data;
		        }
		        need_redraw = info_changed = 1;
	 	    }
		    else
		    {
		        tag->ti_Tag = TAG_IGNORE;
		    }
		    break;

	    case    MUIA_Gauge_Max:
	    	    if (data->max != tag->ti_Data)
		    {
	    	        data->max = tag->ti_Data;
		        need_redraw = 1;
		    }
		    else
		    {
		        tag->ti_Tag = TAG_IGNORE;
		    }
		    break;
    	}
    }

    if (data->divide != 0)
	data->current /= data->divide;
    if (data->current > data->max)
	data->current = data->max;

    if (info_changed)
    {
	if (data->info)
	{
#ifdef _AROS
	snprintf(data->buf, GAUGE_BUFSIZE, data->info, data->current);
#else
	sprintf(data->buf, data->info, data->current);
#endif
	} else data->buf[0] = 0;
    }

    if (need_redraw)
    {
    	MUI_Redraw(obj,MADF_DRAWOBJECT);
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG  Gauge_Get(struct IClass *cl, Object * obj, struct opGet *msg)
{
    struct MUI_GaugeData *data = INST_DATA(cl, obj);
    ULONG *store               = msg->opg_Storage;
    ULONG    tag               = msg->opg_AttrID;

    switch (tag)
    {
        case MUIA_Gauge_Current:
	    *store = data->current;
	    break;

	case MUIA_Gauge_Divide:
	    *store = data->divide;
	    break;

	case MUIA_Gauge_InfoText:
	    *store = (ULONG)data->info;
	    break;

	case MUIA_Gauge_Max:
	    *store = data->max;
	    break;

	case MUIA_Gauge_DupInfoText:
	    *store = (ULONG)data->dupinfo;
	    break;

    	default:
	    return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    return TRUE;
}


/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Gauge_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_GaugeData *data = INST_DATA(cl,obj);

    if (!(DoSuperMethodA(cl, obj, (Msg)msg))) return 0;

    if (data->info)
    {
	struct RastPort rp;

	InitRastPort(&rp);
	SetFont(&rp,_font(obj));

	data->info_width = TextLength(&rp,data->buf,strlen(data->buf));
	data->info_height = _font(obj)->tf_YSize;
    } else
    {
	if (data->horiz)
	{
	    data->info_width = 0;
	    data->info_height = 0;
	}
	else
	{
	    data->info_width = 0;
	    data->info_height = 0;
	}
    }

    return 1;
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Gauge_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_GaugeData *data = INST_DATA(cl,obj);
    DoSuperMethodA(cl,obj,(Msg)msg);

    if (data->horiz)
    {
	msg->MinMaxInfo->MinWidth  += data->info_width;
	msg->MinMaxInfo->MinHeight += data->info_height + 2;
	msg->MinMaxInfo->DefWidth  += data->info_width + 10;
	msg->MinMaxInfo->DefHeight += data->info_height + 2;
	msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
	if (data->info)
	    msg->MinMaxInfo->MaxHeight += data->info_height + 2;
	else
	    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    }
    else
    {
	msg->MinMaxInfo->MinWidth  += data->info_width + 2;
	msg->MinMaxInfo->MinHeight += data->info_height;
	msg->MinMaxInfo->DefWidth  += data->info_width + 2;
	msg->MinMaxInfo->DefHeight += data->info_height + 10;
	if (data->info)
	    msg->MinMaxInfo->MaxWidth += data->info_width + 2;
	else
	    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

	msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;
    }
    return 0;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Gauge_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_GaugeData *data = INST_DATA(cl,obj);
    ULONG val;

    DoSuperMethodA(cl,obj,(Msg)msg);
    D(bug("muimaster.library/gauge.c: Draw Gauge Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    if (data->horiz)
    {
	ULONG w;
        w = _mwidth(obj) * data->current / data->max; /* NOTE: should be 64 bit */

    	SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_FILL],0,JAM1);
    	RectFill(_rp(obj),_mleft(obj),_mtop(obj),_mleft(obj) + w - 1, _mbottom(obj));

	if (data->info)
	{
	    ZText *ztext = zune_text_new("\33c\0338",data->buf,ZTEXT_ARG_NONE,0);
	    if (ztext)
	    {
	    	zune_text_get_bounds(ztext, obj);
		zune_text_draw(ztext, obj, _mleft(obj),_mright(obj),_mtop(obj) + (_mheight(obj) - ztext->height)/2);
		zune_text_destroy(ztext);
	    }
        }
    }
    else
    {
	ULONG h;
        h = _mheight(obj) * data->current / data->max; /* NOTE: should be 64 bit */

    	SetABPenDrMd(_rp(obj),_pens(obj)[MPEN_FILL],0,JAM1);
    	RectFill(_rp(obj),_mleft(obj),_mbottom(obj) - h + 1, _mright(obj),_mbottom(obj));

	if (data->info)
	{
	    ZText *ztext = zune_text_new("\33c\0338",data->buf,ZTEXT_ARG_NONE,0);
	    if (ztext)
	    {
	    	zune_text_get_bounds(ztext, obj);
		zune_text_draw(ztext, obj, _mleft(obj),_mright(obj),_mtop(obj) + (_mheight(obj) - ztext->height)/2);
		zune_text_destroy(ztext);
	    }
        }
    }
    return 0;
}

#ifndef _AROS
__asm IPTR Gauge_Dispatcher( register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR,Gauge_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Gauge_New(cl, obj, (struct opSet *)msg);
	case OM_DISPOSE: return Gauge_Dispose(cl, obj, (Msg)msg);
	case OM_SET: return Gauge_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Gauge_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Setup: return Gauge_Setup(cl, obj, (struct MUIP_Setup *)msg);
	case MUIM_AskMinMax: return Gauge_AskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg);
	case MUIM_Draw: return Gauge_Draw(cl, obj, (struct MUIP_Draw*)msg);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Gauge_desc = { 
    MUIC_Gauge, 
    MUIC_Area, 
    sizeof(struct MUI_GaugeData), 
    (void*)Gauge_Dispatcher 
};

