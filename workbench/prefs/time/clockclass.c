/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "clockclass.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <aros/debug.h>
#include <aros/asmcall.h>

/*********************************************************************************************/

#define SHADOW_OFFX 4
#define SHADOW_OFFY 4

/*********************************************************************************************/

struct MUI_ClockData
{
    struct MUI_InputHandlerNode ihn;
    struct ClockData	    	clockdata;
    struct BitMap   	    	*clockbm;
    struct RastPort 	    	clockrp;
    struct AreaInfo 	    	clockai;
    struct TmpRas   	    	clocktr;
    APTR    	    	    	clockraster;
    WORD    	    	    	areabuf[30];
    WORD    	    	    	clockbmr, clockbmw, clockbmh;
    WORD    	    	    	edithand;
    WORD    	    	    	editpen;
    BOOL    	    	    	frozen;
};


/*********************************************************************************************/

static IPTR Clock_New(Class *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ClockData *data;
    struct TagItem  	*ti, tags[] =
    {
	{MUIA_InnerLeft     , 4 	    	    	},
	{MUIA_InnerTop	    , 4 	    	    	},
	{MUIA_InnerRight    , 4 	    	    	},
	{MUIA_InnerBottom   , 4 	    	     	},
	{TAG_MORE   	    , (IPTR)msg->ops_AttrList	}
    };
    
    msg->ops_AttrList = tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
	
    if (!obj) return 0;
    
    data = INST_DATA(cl, obj);
    data->frozen = GetTagData(MUIA_Clock_Frozen, FALSE, msg->ops_AttrList) ? TRUE : FALSE;
    data->edithand = (WORD)GetTagData(MUIA_Clock_EditHand, (IPTR)-1, msg->ops_AttrList);
    data->editpen = -1;
    
    if ((ti = FindTagItem(MUIA_Clock_Time, msg->ops_AttrList)))
    {
    	struct ClockData *cd = (struct ClockData *)ti->ti_Data;
	
	data->clockdata = *cd;
    }
    else
    {
    	struct timeval tv;
	
    	GetSysTime(&tv);
	Amiga2Date(tv.tv_secs, &data->clockdata);
    }
    
    data->ihn.ihn_Flags  = MUIIHNF_TIMER;
    data->ihn.ihn_Method = MUIM_Clock_Timer;
    data->ihn.ihn_Object = obj;
    data->ihn.ihn_Millis = 1000;
    
    return (IPTR)obj;
}

/*********************************************************************************************/

static IPTR Clock_Dispose(Class *cl, Object *obj, Msg msg)
{
    return DoSuperMethodA(cl, obj, msg);
}

/*********************************************************************************************/

static IPTR Clock_Set(Class *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ClockData *data = INST_DATA(cl, obj);
    struct ClockData  	  old_clockdata;
    struct TagItem   	 *tags  = msg->ops_AttrList;
    struct TagItem   	 *tag;
    BOOL    	      	  redraw = FALSE;
    
    old_clockdata = data->clockdata;
    
    while ((tag = NextTagItem(&tags)) != NULL)
    {
    	switch(tag->ti_Tag)
	{
    	    case MUIA_Clock_Time:
		data->clockdata = *(struct ClockData *)tag->ti_Data;
		redraw = TRUE;
		break;

	    case MUIA_Clock_Hour:
		data->clockdata.hour = tag->ti_Data;
		redraw = TRUE;
		break;

	    case MUIA_Clock_Min:
		data->clockdata.min = tag->ti_Data;
		redraw = TRUE;
		break;

	    case MUIA_Clock_Sec:
		data->clockdata.sec = tag->ti_Data;
		redraw = TRUE;
		break;

    	    case MUIA_Clock_Frozen:
	    	data->frozen = tag->ti_Data ? TRUE : FALSE;
		break;
		
	    case MUIA_Clock_EditHand:
	    	data->edithand = tag->ti_Data;
		redraw = TRUE;
		break;
		
	} /* switch(tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem(&tags)) != NULL) */
    
    if (redraw)
    {
    	 MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*********************************************************************************************/

static IPTR Clock_Get(Class *cl, Object *obj, struct opGet *msg)
{
    struct MUI_ClockData *data = INST_DATA(cl, obj);
    IPTR    	      retval = TRUE;
    
    switch(msg->opg_AttrID)
    {
    	case MUIA_Clock_Time:
	    *(struct ClockData **)msg->opg_Storage = &data->clockdata;
	    break;

	case MUIA_Clock_Hour:
	    *msg->opg_Storage = data->clockdata.hour;
	    break;

	case MUIA_Clock_Min:
	    *msg->opg_Storage = data->clockdata.min;
	    break;
	    
	case MUIA_Clock_Sec:
	    *msg->opg_Storage = data->clockdata.sec;
	    break;

    	case MUIA_Clock_Frozen:
	    *msg->opg_Storage = data->frozen;
	    break;
	    
	case MUIA_Clock_EditHand:
	    *msg->opg_Storage = data->edithand;
	    break;
	    
    	default:
	    retval = DoSuperMethodA(cl, obj, (Msg)msg);
	    break;
    }
    
    return retval;
}

/*********************************************************************************************/

static IPTR Clock_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ClockData *data = INST_DATA(cl, obj);
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    DoMethod(_app(obj), MUIM_Application_AddInputHandler, &data->ihn);
    
    data->editpen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
    	    	    	    	  0xFFFFFFFF,
				  0xD8D8D8D8,
				  0x00000000,
				  OBP_Precision, PRECISION_GUI,
				  OBP_FailIfBad, FALSE,
				  TAG_DONE);
    return TRUE;
}

/*********************************************************************************************/

static IPTR Clock_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ClockData *data = INST_DATA(cl, obj);
 
    if (data->editpen != -1)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->editpen);
    	data->editpen = -1;
    }
    
    if (data->clockbm)
    {
    	DeinitRastPort(&data->clockrp);
    	FreeBitMap(data->clockbm);
	FreeRaster(data->clockraster, data->clockbmw, data->clockbmh);
	data->clockbm = NULL;
	data->clockraster = NULL;
    }
    DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->ihn);
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*********************************************************************************************/

static IPTR Clock_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    msg->MinMaxInfo->MinWidth  += 31;
    msg->MinMaxInfo->MinHeight += 31;
    msg->MinMaxInfo->DefWidth  += 31;
    msg->MinMaxInfo->DefHeight += 31;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;

    return TRUE;
}

/*********************************************************************************************/

#define MY_PI 3.14159265358979323846

/*********************************************************************************************/

static void DrawHand(struct RastPort *rp, WORD cx, WORD cy, DOUBLE angle,
    	    	     WORD radius1, WORD radius2)
{
    WORD x, y;
    
    x = cx + cos(angle + MY_PI) * radius1;
    y = cy - sin(angle + MY_PI) * radius1;
    
    AreaMove(rp, x, y);

    x = cx + cos(angle + MY_PI / 2.0) * radius1;
    y = cy - sin(angle + MY_PI / 2.0) * radius1;
    
    AreaDraw(rp, x, y);
    
    x = cx + cos(angle) * radius2;
    y = cy - sin(angle) * radius2;
    
    AreaDraw(rp, x, y);
    
    x = cx + cos(angle - MY_PI / 2.0) * radius1;
    y = cy - sin(angle - MY_PI / 2.0) * radius1;
     
    AreaDraw(rp, x, y);
    
    x = cx + cos(angle + MY_PI) * radius1;
    y = cy - sin(angle + MY_PI) * radius1;
    
    AreaDraw(rp, x, y);
    
    AreaEnd(rp);
}

/*********************************************************************************************/

static void DrawThinHand(struct RastPort *rp, WORD cx, WORD cy, DOUBLE angle, WORD radius)
{
    WORD x, y;
    
    Move(rp, cx, cy);
    
    x = cx + cos(angle) * radius;
    y = cy - sin(angle) * radius;
    
    Draw(rp, x, y);
}

/*********************************************************************************************/

static IPTR Clock_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_ClockData *data = INST_DATA(cl, obj);
    struct RastPort 	 *obj_rp;
    struct Region   	 *region;
    struct Rectangle	 rect;
    APTR    	    	 clip;
    WORD    	    	 r, x, y, x2, y2, i, cx, cy, c;
    WORD    	    	 new_clockbmw, new_clockbmh;
    WORD    	    	 clock_posx, clock_posy;
    DOUBLE  	    	 angle;

    r = (_mwidth(obj) - SHADOW_OFFX) / 2;
    y = (_mheight(obj) - SHADOW_OFFY) / 2;
    
    r = (r < y) ? r : y;
    
    new_clockbmw = r * 2 + 1 + SHADOW_OFFX;
    new_clockbmh = r * 2 + 1 + SHADOW_OFFY;
    
    clock_posx = _mleft(obj) + (_mwidth(obj) - new_clockbmw) / 2;
    clock_posy = _mtop(obj) + (_mheight(obj) - new_clockbmh) / 2;
    
    region = NewRegion();
    if (region)
    {
    	rect.MinX = _left(obj);
	rect.MinY = _top(obj);
	rect.MaxX = _right(obj);
	rect.MaxY = _bottom(obj);
	
	OrRectRegion(region, &rect);
	
	rect.MinX = clock_posx;
	rect.MinY = clock_posy;
	rect.MaxX = clock_posx + new_clockbmw - 1;
	rect.MaxY = clock_posy + new_clockbmh - 1;
	
	ClearRectRegion(region, &rect);
	
	clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
    }
    
    DoSuperMethodA(cl, obj, (Msg)msg);
  
    if (region)
    {
    	MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    }
    
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 0;
        
    if (!data->clockbm || (data->clockbmr != r))
    {
    	if (data->clockbm)
	{
	    DeinitRastPort(&data->clockrp);
	    FreeBitMap(data->clockbm);
	    FreeRaster(data->clockraster, data->clockbmw, data->clockbmh);
	    data->clockraster = NULL;
	}

	data->clockbmw = new_clockbmw;
	data->clockbmh = new_clockbmh;
    	
    	data->clockbm = AllocBitMap(data->clockbmw,
	    	    	    	    data->clockbmh,
				    GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH),
				    BMF_MINPLANES,
				    _rp(obj)->BitMap);

    	if (!data->clockbm) return 0;
	
	data->clockraster = AllocRaster(data->clockbmw, data->clockbmh);
	if (!data->clockraster)
	{
	    FreeBitMap(data->clockbm);
	    data->clockbm = 0;
	    return 0;
	}
	
	InitRastPort(&data->clockrp);
	data->clockrp.BitMap = data->clockbm;
	
	InitArea(&data->clockai, data->areabuf, sizeof(data->areabuf) / 5);
	data->clockrp.AreaInfo = &data->clockai;
	
	InitTmpRas(&data->clocktr, data->clockraster, RASSIZE(data->clockbmw, data->clockbmh));
	data->clockrp.TmpRas = &data->clocktr;
	
	data->clockbmr = r;  
    }
    
    obj_rp = _rp(obj);
    _rp(obj) = &data->clockrp;

    DoMethod(obj, MUIM_DrawBackground, 0, 0, data->clockbmw , data->clockbmh, clock_posx, clock_posy, 0);
   
    cx = r + SHADOW_OFFX;
    cy = r + SHADOW_OFFY;
    
    SetDrMd(_rp(obj), JAM1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);

    for(c = 0; c < 2; c++)
    {
	for(angle = 0.0, i = 0; angle < 2.0 * MY_PI; angle += 2 * MY_PI / 60.0, i++)
	{
    	    x = cx + cos(angle) * r;
	    y = cy - sin(angle) * r;

	    if ((i % 5) == 0)
	    {
		x2 = cx + cos(angle) * (r * 90 / 100);
		y2 = cy - sin(angle) * (r * 90 / 100);
		Move(_rp(obj), x, y); Draw(_rp(obj), x2, y2);
	    }
	    else
	    {
		WritePixel(_rp(obj), x, y);
	    }
	}
	
	cx -= SHADOW_OFFX;
	cy -= SHADOW_OFFY;
	SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
    }
    
    cx += SHADOW_OFFX * 2;
    cy += SHADOW_OFFY * 2;
    
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);
    
    for(c = 0; c < 2; c++)
    {
    	DOUBLE angle, h;
	
    	h = data->clockdata.hour + (data->clockdata.min / 60.0);
	if (h > 12.0) h -= 12.0;
	
	angle = MY_PI / 2.0 + MY_PI * 2.0 * (12.0 - h) / 12.0;
	
	if (c == 1)
	{
	    if ((data->edithand == EDITHAND_HOUR) && data->frozen)
	    {
	    	SetAPen(_rp(obj), (data->editpen == -1) ? _dri(obj)->dri_Pens[SHINEPEN] : data->editpen);
	    }
	    else
	    {
	    	SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
	    }
	}
	    
	DrawHand(_rp(obj),
	    	 cx,
		 cy,
		 angle,
		 r * 6 / 100,
		 r * 60 / 100);

    	angle = MY_PI / 2.0 + MY_PI * 2.0 * ((double)60 - data->clockdata.min) / 60.0;

	if (c == 1)
	{
	    if ((data->edithand == EDITHAND_MIN) && data->frozen)
	    {
	    	SetAPen(_rp(obj), (data->editpen == -1) ? _dri(obj)->dri_Pens[SHINEPEN] : data->editpen);
	    }
	    else
	    {
	    	SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
	    }
	}

	DrawHand(_rp(obj),
	    	 cx,
		 cy,
		 angle,
		 r * 4 / 100,
		 r * 85 / 100);
		 
    	angle = MY_PI / 2.0 + MY_PI * 2.0 * ((double)60 - data->clockdata.sec) / 60.0;

	if (c == 1)
	{
	    if ((data->edithand == EDITHAND_SEC) && data->frozen)
	    {
	    	SetAPen(_rp(obj), (data->editpen == -1) ? _dri(obj)->dri_Pens[SHINEPEN] : data->editpen);
	    }
	    else
	    {
	    	SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
	    }
	}

	DrawThinHand(_rp(obj), cx, cy, angle, r * 85 / 100);

    	cx -= SHADOW_OFFX;
	cy -= SHADOW_OFFY;	
	
    }
    
    _rp(obj) = obj_rp;

    BltBitMapRastPort(data->clockbm,
    	    	      0,
		      0,
    	    	      _rp(obj),
		      clock_posx,
		      clock_posy,
		      data->clockbmw,
		      data->clockbmh, 192);
    
    return 0;
}

/*********************************************************************************************/

IPTR Clock_Timer(Class *cl, Object *obj, Msg msg)
{
    struct MUI_ClockData *data;
    struct ClockData 	  cd;
    struct timeval  	  tv;
    
    data = INST_DATA(cl, obj);
    
    if (!data->frozen)
    {
	GetSysTime(&tv);
	Amiga2Date(tv.tv_secs, &cd);

	set(obj, MUIA_Clock_Time, &cd);
	set(obj, MUIA_Clock_Ticked, TRUE);
    }
    
    return 0;
}

/*********************************************************************************************/

AROS_UFH3S(IPTR, Clock_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    IPTR retval = 0;
 
    switch (msg->MethodID)
    {
	case OM_NEW:
	    retval = Clock_New(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = Clock_Dispose(cl, obj, msg);
	    break;
	    
	case OM_SET:
	    retval = Clock_Set(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_GET:
	    retval = Clock_Get(cl, obj, (struct opGet *)msg);
    	    break;
	    
	case MUIM_Setup:
	    retval = Clock_Setup(cl, obj, (struct MUIP_Setup *)msg);
	    break;
	    
	case MUIM_Cleanup:
	    retval = Clock_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	    break;
	    
	case MUIM_AskMinMax:
	    retval = Clock_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
	    break;
	    
	case MUIM_Draw:
	    retval = Clock_Draw(cl, obj, (struct MUIP_Draw *)msg);
	    break;
	    
	case MUIM_Clock_Timer:
	    retval = Clock_Timer(cl, obj, msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;	       
    }
    
    return retval;
}

/*********************************************************************************************/

BOOL MakeClockClass(void)
{
    clockmcc = MUI_CreateCustomClass(NULL, MUIC_Area, NULL,
    	    	    	    	     sizeof(struct MUI_ClockData), Clock_Dispatcher);    
    return clockmcc ? TRUE : FALSE;
}

/*********************************************************************************************/

void KillClockClass(void)
{
    if (clockmcc) MUI_DeleteCustomClass(clockmcc);
}

/*********************************************************************************************/
