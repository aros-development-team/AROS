/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "calendarclass.h"

#include <string.h>
#include <stdio.h>

#include <aros/debug.h>
#include <aros/asmcall.h>

/*********************************************************************************************/

#define CELL_EXTRAWIDTH 6
#define CELL_EXTRAHEIGHT 4

/*********************************************************************************************/

struct CalendarData
{
    struct MUI_EventHandlerNode ehn;
    struct ClockData	    	clockdata;
    STRPTR  	    	    	*daylabels;
    STRPTR  	    	    	defdaylabels[12];
    WORD    	    	    	cellwidth;
    WORD    	    	    	cellheight;
};

/*********************************************************************************************/

STRPTR def_daylabels[] =
{
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

/*********************************************************************************************/

static IPTR Calendar_New(Class *cl, Object *obj, struct opSet *msg)
{
    struct CalendarData *data;
    struct TagItem  	*ti, tags[] =
    {
    	{MUIA_Background, MUII_ButtonBack   	    },
	{MUIA_Frame 	, MUIV_Frame_Button 	    },
	{TAG_MORE   	, (IPTR)msg->ops_AttrList   }
    };
    
    msg->ops_AttrList = tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
	
    if (!obj) return 0;
    
    data = INST_DATA(cl, obj);

    data->daylabels = (STRPTR *)GetTagData(MUIA_Calendar_DayLabels, 0, msg->ops_AttrList);
    if (!data->daylabels)
    {
    	struct Locale 	*locale;
	WORD	    	i;
	
	locale = LocaleBase ? OpenLocale(NULL) : NULL;
	
    	data->daylabels = data->defdaylabels;
	for(i = 0; i < 12; i++)
	{
	    if (locale)
	    {
	    	data->defdaylabels[i] = GetLocaleStr(locale, ABDAY_1 + i);
	    }
	    else
	    {
	    	data->defdaylabels[i] = def_daylabels[i];
	    }
	}
	
	if (locale) CloseLocale(locale);
    }
    
    if ((ti = FindTagItem(MUIA_Calendar_Date, msg->ops_AttrList)))
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
    
    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;
    
    return (IPTR)obj;
}

/*********************************************************************************************/

static IPTR Calendar_Dispose(Class *cl, Object *obj, Msg msg)
{
    return DoSuperMethodA(cl, obj, msg);
}

/*********************************************************************************************/

static IPTR Calendar_Set(Class *cl, Object *obj, struct opSet *msg)
{
    struct CalendarData *data = INST_DATA(cl, obj);
    struct TagItem      *tags  = msg->ops_AttrList;
    struct TagItem      *tag;
    BOOL    	    	 redraw = FALSE;
    
kprintf("calendar_set\n");
    while ((tag = NextTagItem(&tags)) != NULL)
    {
    	switch(tag->ti_Tag)
	{
    	    case MUIA_Calendar_Date:
		data->clockdata = *(struct ClockData *)tag->ti_Data;
		redraw = TRUE;
		break;

	    case MUIA_Calendar_Year:
		data->clockdata.year = tag->ti_Data;
		redraw = TRUE;
		break;

	    case MUIA_Calendar_Month:
		data->clockdata.month = tag->ti_Data;
		redraw = TRUE;
		break;

	    case MUIA_Calendar_Month0:
		data->clockdata.month = tag->ti_Data + 1;
		redraw = TRUE;
		break;

	    case MUIA_Calendar_MonthDay:
		data->clockdata.mday = tag->ti_Data;
		redraw = TRUE;
		break;

	    case MUIA_Calendar_MonthDay0:
		data->clockdata.mday = tag->ti_Data + 1;
		redraw = TRUE;
		break;
		
	} /* switch(tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem(&tags)) != NULL) */
    
    if (redraw)
    {
    	ULONG secs = Date2Amiga(&data->clockdata);
	
	Amiga2Date(secs, &data->clockdata);
	
    	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*********************************************************************************************/

static IPTR Calendar_Get(Class *cl, Object *obj, struct opGet *msg)
{
    struct CalendarData *data = INST_DATA(cl, obj);
    IPTR    	    	retval = TRUE;
    
    switch(msg->opg_AttrID)
    {
    	case MUIA_Calendar_Date:
	    *(struct ClockData **)msg->opg_Storage = &data->clockdata;
	    break;

	case MUIA_Calendar_MonthDay:
	    *msg->opg_Storage = data->clockdata.mday;
	    break;

	case MUIA_Calendar_MonthDay0:
	    *msg->opg_Storage = data->clockdata.mday - 1;
	    break;
	    
	case MUIA_Calendar_Month:
	    *msg->opg_Storage = data->clockdata.month;
	    break;

	case MUIA_Calendar_Month0:
	    *msg->opg_Storage = data->clockdata.month - 1;
	    break;
	    
	case MUIA_Calendar_Year:
	    *msg->opg_Storage = data->clockdata.year;
	    break;
	    
    	default:
	    retval = DoSuperMethodA(cl, obj, (Msg)msg);
	    break;
	    
    }
    
    return retval;
}

/*********************************************************************************************/

static IPTR Calendar_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct CalendarData *data = INST_DATA(cl, obj);
    struct RastPort 	 rp;
    WORD    	    	 i, w;
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    InitRastPort(&rp);
    SetFont(&rp, _font(obj));
    
    data->cellheight = _font(obj)->tf_YSize + CELL_EXTRAHEIGHT;
    
    SetSoftStyle(&rp, FSF_BOLD, AskSoftStyle(&rp));
    
    for(i = 0; i < 7; i++)
    {
    	w = TextLength(&rp, data->daylabels[i], strlen(data->daylabels[i]));
	if (w > data->cellwidth) data->cellwidth = w;
    }
    
    SetSoftStyle(&rp, FS_NORMAL, AskSoftStyle(&rp));
 
    for(i = 1; i <= 31; i++)
    {
    	char s[3];
	
	sprintf(s, "%d", i);
	
	w = TextLength(&rp, s, strlen(s));
	if (w > data->cellwidth) data->cellwidth = w;
    }
    
    data->cellwidth += CELL_EXTRAWIDTH;
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);
    
    return TRUE;
}

/*********************************************************************************************/

static IPTR Calendar_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct CalendarData *data = INST_DATA(cl, obj);
 
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*********************************************************************************************/

static IPTR Calendar_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct CalendarData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);
    
    msg->MinMaxInfo->MinWidth  += data->cellwidth * 7;
    msg->MinMaxInfo->MinHeight += data->cellheight * 7;
    msg->MinMaxInfo->DefWidth  += data->cellwidth * 7;
    msg->MinMaxInfo->DefHeight += data->cellheight * 7;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;
    
    
    return TRUE;
}

/*********************************************************************************************/

static IPTR Calendar_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct CalendarData *data = INST_DATA(cl, obj);
    WORD    	    	 x, y, offx, offy, mwday, day, mdays;
    
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 0;
    
    if (msg->flags & MADF_DRAWUPDATE)
    {
    }
    
    offx = (_mwidth(obj) - data->cellwidth * 7) / 2;
    offy = (_mheight(obj) - data->cellheight * 7) / 2;
    
    /* ~random 8 * 7 to make sure expression inside brackets is positive */
    mwday = (8 * 7 + data->clockdata.wday - data->clockdata.mday + 1) % 7;
    
    mdays = NumMonthDays(&data->clockdata);

kprintf("actdate = %d.%d.%d  wday = %d\n", data->clockdata.mday, data->clockdata.month, data->clockdata.year, data->clockdata.wday);

kprintf("mwday = %d  mdays = %d\n", mwday, mdays);
    
    day = firstweekday - mwday + 1 - 7;
    
    SetFont(_rp(obj), _font(obj));
    SetDrMd(_rp(obj), JAM1);
    
    for(y = 0; y < 7; y++)
    {
    	for(x = 0; x < 7; x++)
	{
	    STRPTR text;	    
	    UBYTE buf[3];
	    
	    text = NULL;
	    SetAPen(_rp(obj), _dri(obj)->dri_Pens[y ? SHINEPEN : SHADOWPEN]);
	    RectFill(_rp(obj), _mleft(obj) + x * data->cellwidth + 1 + offx,
	    	    	       _mtop(obj) + y * data->cellheight + 1 + offy,
			       _mleft(obj) + x * data->cellwidth + data->cellwidth - 2 + offx,
			       _mtop(obj) + y * data->cellheight + data->cellheight - 2 + offy);
			       
	    SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
	    Move(_rp(obj), _mleft(obj) + x * data->cellwidth + offx,
		    	   _mtop(obj) + y * data->cellheight + offy);
	    Draw(_rp(obj), _mleft(obj) + x * data->cellwidth + data->cellwidth - 1 + offx,
		    	   _mtop(obj) + y * data->cellheight + offy);
	    Draw(_rp(obj), _mleft(obj) + x * data->cellwidth + data->cellwidth - 1 + offx,
		    	   _mtop(obj) + y * data->cellheight + data->cellheight - 1 + offy);
	    Draw(_rp(obj), _mleft(obj) + x * data->cellwidth + offx,
		    	   _mtop(obj) + y * data->cellheight + data->cellheight - 1 + offy);
	    Draw(_rp(obj), _mleft(obj) + x * data->cellwidth + offx,
		    	   _mtop(obj) + y * data->cellheight + offy);

    	    if (y > 0)
	    {
	    	if ((day >= 1) && (day <= mdays))
		{
		    sprintf(buf, "%d", day);
	    	    SetSoftStyle(_rp(obj), FS_NORMAL, AskSoftStyle(_rp(obj)));
		    SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
		    text = buf;
		}
	    }
	    else
	    {
	    	SetSoftStyle(_rp(obj), FSF_BOLD, AskSoftStyle(_rp(obj)));
		SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHINEPEN]);
		
		text = data->daylabels[(x + firstweekday) % 7];
	    }
	    
	    if (text)
	    {
	    	WORD tx, ty, tw;
		
		tw = TextLength(_rp(obj), text, strlen(text));
		tx = _mleft(obj) + offx + x * data->cellwidth + (data->cellwidth - tw) / 2;
		ty = _mtop(obj) + offy + y * data->cellheight + (data->cellheight - _font(obj)->tf_YSize) / 2;
		
		Move(_rp(obj), tx, ty + _font(obj)->tf_Baseline);
		Text(_rp(obj), text, strlen(text));
	    }
	    
	    day++;
	    
	} /* for(x = 0; x < 7; x++) */
	
    } /* for(y = 0; y < 7; y++) */
    
    return 0;
}

/*********************************************************************************************/

static IPTR Calendar_HandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    return 0;
}

/*********************************************************************************************/

AROS_UFH3S(IPTR, Calendar_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    IPTR retval = 0;
    
    switch (msg->MethodID)
    {
	case OM_NEW:
	    retval = Calendar_New(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_DISPOSE:
	    retval = Calendar_Dispose(cl, obj, msg);
	    break;
	    
	case OM_SET:
	    retval = Calendar_Set(cl, obj, (struct opSet *)msg);
	    break;
	    
	case OM_GET:
	    retval = Calendar_Get(cl, obj, (struct opGet *)msg);
    	    break;
	    
	case MUIM_Setup:
	    retval = Calendar_Setup(cl, obj, (struct MUIP_Setup *)msg);
	    break;
	    
	case MUIM_Cleanup:
	    retval = Calendar_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	    break;
	    
	case MUIM_AskMinMax:
	    retval = Calendar_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
	    break;
	    
	case MUIM_Draw:
	    retval = Calendar_Draw(cl, obj, (struct MUIP_Draw *)msg);
	    break;
	    
	case MUIM_HandleEvent:
	    retval = Calendar_HandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;	       
    }
    
    return retval;
}

/*********************************************************************************************/

BOOL MakeCalendarClass(void)
{
    calendarmcc = MUI_CreateCustomClass(NULL, MUIC_Area, NULL,
    	    	    	    	    	sizeof(struct CalendarData), Calendar_Dispatcher);    
    return calendarmcc ? TRUE : FALSE;
}

/*********************************************************************************************/

void KillCalendarClass(void)
{
    if (calendarmcc) MUI_DeleteCustomClass(calendarmcc);
}

/*********************************************************************************************/
