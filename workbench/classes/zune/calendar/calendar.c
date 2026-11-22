/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <utility/date.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/timer.h>

#include <aros/debug.h>
#include <aros/asmcall.h>

#include <string.h>
#include <stdio.h>

#include "calendar.h"
#include "calendar_private.h"

#define CELL_EXTRAWIDTH 6
#define CELL_EXTRAHEIGHT 4

static STRPTR def_daylabels[] =
{
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

/****** Calendar.mcc/MUIC_Calendar ************
*
*   NAME
*       MUIC_Calendar
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Draws a month grid with the selected day highlighted. Date is
*       initialized from a copied ClockData or current system time. Keyboard
*       and mouse selection stays in the current month; no month navigation
*       or periodic date refresh is provided.
*
*   NOTES
*       Date is copied on input; Get borrows instance data. DayLabels borrows
*       seven Sunday-first strings. Setup/Cleanup register/remove a
*       Window event handler. Date fields are checked before conversion.
*       Invalid initial dates fail object creation; invalid complete-date
*       updates leave the date unchanged. Component setters can cap an
*       oversized day. Dates must fit the 32-bit AROS epoch.
*
*/
/****** Calendar.mcc/MUIA_Calendar_Date ************
*
*   NAME
*       MUIA_Calendar_Date
*
*   FUNCTION
*       Get or replace the complete ClockData date.
*
*   INPUTS
*       Initialize, set, get; non-NULL ClockData pointer for input.
*
*   NOTES
*       Input is copied; Get borrows the instance address. A complete date
*       provided during Set must be valid and representable; it is not
*       silently capped. Invalid or NULL date inputs leave the current
*       date unchanged; invalid initial dates fail object construction.
*       Component setters can cap a day to the end of the month. The
*       weekday is recomputed from a validated date.
*       Only date changes redraw; notifications carry original input, not
*       necessarily normalized data. Mouse/keyboard updates notify MonthDay.
*
*/
/****** Calendar.mcc/MUIA_Calendar_MonthDay ************
*
*   NAME
*       MUIA_Calendar_MonthDay
*
*   FUNCTION
*       Get or set the one-based day of month.
*
*   INPUTS
*       Set, get; UWORD, 1..number of days in month.
*
*   NOTES
*       An oversized value is capped at month end during Set; invalid
*       or unrepresentable values are ignored. No constructor handling. A changed date
*       redraws; Date and alias notifications are not synthesized.
*
*/
/****** Calendar.mcc/MUIA_Calendar_MonthDay0 ************
*
*   NAME
*       MUIA_Calendar_MonthDay0
*
*   FUNCTION
*       Get or set the zero-based day of month.
*
*   INPUTS
*       Set, get; UWORD, 0..days-in-month minus one.
*
*   NOTES
*       Set adds one; Get subtracts one. Excessive days are capped during
*       Set; invalid or unrepresentable values are ignored. No
*       constructor handling or synthesized Date/alias notification.
*
*/
/****** Calendar.mcc/MUIA_Calendar_Month ************
*
*   NAME
*       MUIA_Calendar_Month
*
*   FUNCTION
*       Get or set the one-based month.
*
*   INPUTS
*       Set, get; UWORD, 1..12.
*
*   NOTES
*       Day is capped to the selected month length and date reconverted.
*       Out-of-range months are ignored. No
*       constructor handling or synthesized Date/alias notification.
*
*/
/****** Calendar.mcc/MUIA_Calendar_Month0 ************
*
*   NAME
*       MUIA_Calendar_Month0
*
*   FUNCTION
*       Get or set the zero-based month.
*
*   INPUTS
*       Set, get; UWORD, 0..11.
*
*   NOTES
*       Set adds one; Get subtracts one. Day is capped to month length
*       and date reconverted. Out-of-range months are ignored; no constructor
*       handling or synthesized Date/alias notification.
*
*/
/****** Calendar.mcc/MUIA_Calendar_Year ************
*
*   NAME
*       MUIA_Calendar_Year
*
*   FUNCTION
*       Get or set the calendar year.
*
*   INPUTS
*       Set, get; UWORD, representable full year.
*
*   NOTES
*       Day is capped and date reconverted. Unrepresentable years are
*       ignored; no constructor handling or synthesized Date/alias notification.
*
*/
/****** Calendar.mcc/MUIA_Calendar_DayLabels ************
*
*   NAME
*       MUIA_Calendar_DayLabels
*
*   FUNCTION
*       Supply the seven weekday captions.
*
*   INPUTS
*       Initialize only; pointer to seven NUL-terminated strings in
*       Sunday-to-Saturday order. NULL uses defaults.
*
*   NOTES
*       Array and strings are borrowed and must remain valid while used.
*       NULL elements are not checked. No local setter/getter. Custom labels
*       use Sunday-first
*       ordering instead of the default locale-based calendar ordering.
*
*/

/*** Methods ****************************************************************/

/* Reject unrepresentable dates, then recompute the weekday using AROS utility.
 * Date2Amiga has a 32-bit result; the round trip also rejects overflow.
 * CheckDate alone cannot distinguish an invalid date from the epoch (0).
 */
static BOOL Calendar_NormalizeDate(struct ClockData *date);

/*
 * Borrow caller labels or retain the locale for built-in labels.
 * Validate the initial date and prepare Window events for Setup.
 */
IPTR Calendar__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct Calendar_DATA *data;
    struct TagItem       *ti;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return 0;
    
    data = INST_DATA(cl, obj);
    data->firstweekday = 0;
    data->locale = NULL;

    data->daylabels = (CONST_STRPTR *)GetTagData(MUIA_Calendar_DayLabels, 0, msg->ops_AttrList);
    if (!data->daylabels)
    {
        struct Locale   *locale;
        WORD            i;
        
        locale = LocaleBase ? OpenLocale(NULL) : NULL;
        
        data->daylabels = data->defdaylabels;
        for(i = 0; i < 7; i++)
        {
            if (locale)
            {
                data->defdaylabels[i] = GetLocaleStr(locale, ABDAY_1 + i);
                data->firstweekday = locale->loc_CalendarType;
            }
            else
            {
                data->defdaylabels[i] = def_daylabels[i];
                data->firstweekday = 0;
            }
        }
        
        /* GetLocaleStr strings belong to the open Locale. */
        data->locale = locale;
    }
    
    if ((ti = FindTagItem(MUIA_Calendar_Date, msg->ops_AttrList)))
    {
        struct ClockData *cd = (struct ClockData *)ti->ti_Data;
        
        if (!cd)
        {
            CoerceMethod(cl, obj, OM_DISPOSE);
            return 0;
        }
        data->clockdata = *cd;
    }
    else
    {
        struct timeval tv;
        
        GetSysTime(&tv);
        Amiga2Date(tv.tv_secs, &data->clockdata);
    }

    if (!Calendar_NormalizeDate(&data->clockdata))
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;
    
    return (IPTR)obj;
}


/*
 * Close a retained locale and delegate Area disposal. Caller-provided
 * label storage remains caller-owned. Requires prior Cleanup after Setup.
 */
IPTR Calendar__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);

    if (data->locale)
    {
        CloseLocale(data->locale);
        data->locale = NULL;
    }
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Return the Gregorian month length, or zero for an invalid month.
 */
LONG NumMonthDays(struct ClockData *cd)
{
    static const UBYTE days[] =
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    LONG count;

    if (!cd || cd->month < 1 || cd->month > 12)
        return 0;

    count = days[cd->month - 1];
    if (cd->month == 2 &&
        (cd->year % 400 == 0 ||
         (cd->year % 4 == 0 && cd->year % 100 != 0)))
        ++count;
    return count;
}

/* Validate Gregorian fields and the 32-bit AROS date representation. */
static BOOL Calendar_NormalizeDate(struct ClockData *date)
{
    struct ClockData normalized;
    ULONG seconds;

    if (!date || date->year < 1978 ||
        date->month < 1 || date->month > 12 ||
        date->mday < 1 || date->mday > NumMonthDays(date) ||
        date->hour > 23 || date->min > 59 || date->sec > 59)
        return FALSE;

    seconds = Date2Amiga(date);
    Amiga2Date(seconds, &normalized);
    if (date->year != normalized.year ||
        date->month != normalized.month ||
        date->mday != normalized.mday ||
        date->hour != normalized.hour ||
        date->min != normalized.min ||
        date->sec != normalized.sec)
        return FALSE;

    *date = normalized;
    return TRUE;
}

/*
 * Apply valid date tags and recompute weekday. Reject invalid complete
 * dates without changing state; component setters may cap an oversized
 * day. Notifications retain original tag values.
 */
IPTR Calendar__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);
    struct ClockData old_clockdata = data->clockdata;
    struct ClockData new_clockdata = old_clockdata;
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;
    BOOL update = FALSE;
    BOOL valid = TRUE;
    BOOL full_date = FALSE;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Calendar_Date:
                full_date = TRUE;
                if (tag->ti_Data)
                    new_clockdata = *(struct ClockData *)tag->ti_Data;
                else
                    valid = FALSE;
                update = TRUE;
                break;
            case MUIA_Calendar_Year:
                if (tag->ti_Data > 65535)
                    valid = FALSE;
                else
                    new_clockdata.year = tag->ti_Data;
                update = TRUE;
                break;
            case MUIA_Calendar_Month:
                if (tag->ti_Data < 1 || tag->ti_Data > 12)
                    valid = FALSE;
                else
                    new_clockdata.month = tag->ti_Data;
                update = TRUE;
                break;
            case MUIA_Calendar_Month0:
                if (tag->ti_Data > 11)
                    valid = FALSE;
                else
                    new_clockdata.month = tag->ti_Data + 1;
                update = TRUE;
                break;
            case MUIA_Calendar_MonthDay:
                if (tag->ti_Data > 65535)
                    valid = FALSE;
                else
                    new_clockdata.mday = tag->ti_Data;
                update = TRUE;
                break;
            case MUIA_Calendar_MonthDay0:
                if (tag->ti_Data > 65534)
                    valid = FALSE;
                else
                    new_clockdata.mday = tag->ti_Data + 1;
                update = TRUE;
                break;
        }
    }

    if (update && valid)
    {
        LONG days = NumMonthDays(&new_clockdata);

        if (days)
        {
            if (!full_date && new_clockdata.mday > days)
                new_clockdata.mday = days;

            if (Calendar_NormalizeDate(&new_clockdata))
            {
                data->clockdata = new_clockdata;
                if (new_clockdata.year != old_clockdata.year ||
                    new_clockdata.month != old_clockdata.month ||
                    new_clockdata.mday != old_clockdata.mday)
                {
                    if (new_clockdata.year == old_clockdata.year &&
                        new_clockdata.month == old_clockdata.month)
                        data->old_mday = old_clockdata.mday;

                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                    data->old_mday = 0;
                }
            }
        }
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*
 * Return date components or borrowed ClockData; zero-based getters
 * subtract one. DayLabels has no getter.
 */
IPTR Calendar__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);
    IPTR                 retval = TRUE;
    
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


/*
 * Measure labels/grid afresh and register Window events after Area
 * setup; repeated Setup does not accumulate width.
 */
IPTR Calendar__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);
    struct RastPort      rp;
    WORD                 i, w;
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    InitRastPort(&rp);
    SetFont(&rp, _font(obj));
    data->cellwidth = 0;
    
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
    
    data->base_cellwidth = data->cellwidth;
    data->base_cellheight = data->cellheight;
    
    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->ehn);
    
    return TRUE;
}


/*
 * Remove Window events, clear drag state, and delegate Cleanup.
 * Does not release caller-owned label strings.
 */
IPTR Calendar__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);
 
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->ehn);
    data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/*
 * Size the seven-column grid from measured cells; requires
 * completed Setup and positive dimensions.
 */
IPTR Calendar__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);
    
    msg->MinMaxInfo->MinWidth  += data->cellwidth * 7 + 2;
    msg->MinMaxInfo->MinHeight += data->cellheight * 7 + 1;
    msg->MinMaxInfo->DefWidth  += data->cellwidth * 7 + 2;
    msg->MinMaxInfo->DefHeight += data->cellheight * 7 + 1;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;
    
    
    return TRUE;
}


/*
 * Draw and cache month-grid dimensions; suppress drawing for
 * degenerate geometry, leaving an empty Area background.
 */
IPTR Calendar__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);
    struct Region       *region;
    struct Rectangle     rect;
    APTR                 clip = NULL;
    WORD                 x, y, offx, offy, day, mdays;

    if (data->base_cellwidth <= 0 || data->base_cellheight <= 0 ||
        _mwidth(obj) < 9 || _mheight(obj) < 8)
    {
        data->cellwidth = 0;
        data->cellheight = 0;
        DoSuperMethodA(cl, obj, (Msg)msg);
        return 0;
    }

    x = (_mwidth(obj) - 2) / 7;
    y = x * data->base_cellheight / data->base_cellwidth;
    
    data->cellheight = (_mheight(obj) - 1) / 7;
    data->cellwidth = data->cellheight * data->base_cellwidth / data->base_cellheight;
    
    if ((data->cellwidth > x) || (data->cellheight > y))
    {
        data->cellwidth = x;
        data->cellheight = y;
    }

    if (data->cellwidth <= 0 || data->cellheight <= 0)
    {
        data->cellwidth = 0;
        data->cellheight = 0;
        DoSuperMethodA(cl, obj, (Msg)msg);
        return 0;
    }

    offx = _mleft(obj) + (_mwidth(obj) - data->cellwidth * 7 - 2) / 2 + 1;
    offy = _mtop(obj) + (_mheight(obj) - data->cellheight * 7 - 1) / 2;
    
    region = NewRegion();
    if (region)
    {
        rect.MinX = _left(obj);
        rect.MinY = _top(obj);
        rect.MaxX = _right(obj);
        rect.MaxY = _bottom(obj);
        
        OrRectRegion(region, &rect);
        
        rect.MinX = offx - 1;
        rect.MinY = offy;
        rect.MaxX = offx + data->cellwidth * 7;
        rect.MaxY = offy + data->cellheight * 7;
        
        ClearRectRegion(region, &rect);
        
        clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
    }
    
    DoSuperMethodA(cl, obj, (Msg)msg);

    if (region)
    {
        MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    }
   
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 0;
        
    data->mwday =
        (data->clockdata.wday - (data->clockdata.mday - 1) % 7 + 7) % 7;
    
    mdays = NumMonthDays(&data->clockdata);

    //kprintf("actdate = %d.%d.%d  wday = %d\n", data->clockdata.mday, data->clockdata.month, data->clockdata.year, data->clockdata.wday);

    //kprintf("mwday = %d  mdays = %d\n", data->mwday, mdays);
    
    day = data->firstweekday - data->mwday + 1 - 7;
    if (day > -6) day -= 7;
        
    SetFont(_rp(obj), _font(obj));
    SetDrMd(_rp(obj), JAM1);
    
    if (!(msg->flags & MADF_DRAWUPDATE))
    {
        SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
        Move(_rp(obj), offx - 1,
                       offy);
        Draw(_rp(obj), offx - 1,
                       offy + data->cellheight * 7);
        Draw(_rp(obj), offx +  data->cellwidth * 7,
                       offy + data->cellheight * 7);
        Draw(_rp(obj), offx + data->cellwidth * 7,
                       offy);
    }
    
    for(y = 0; y < 7; y++)
    {
        if (!y && (msg->flags & MADF_DRAWUPDATE))
        {
            day += 7;
            continue;
        }
        
        for(x = 0; x < 7; x++)
        {
            CONST_STRPTR text;
            UBYTE buf[7];
            
            if (data->old_mday && (day != data->old_mday) && (day != data->clockdata.mday))
            {
                day++;
                continue;
            }
            
            text = NULL;
            if (day == data->clockdata.mday)
            {
                SetAPen(_rp(obj), _pens(obj)[MPEN_FILL]);
            }
            else
            {
                SetAPen(_rp(obj), _dri(obj)->dri_Pens[y ? SHINEPEN : SHADOWPEN]);
            }
            
            RectFill(_rp(obj), x * data->cellwidth + 1 + offx,
                               y * data->cellheight + 1 + offy,
                               x * data->cellwidth + data->cellwidth - 2 + offx,
                               y * data->cellheight + data->cellheight - 2 + offy);
                               
            SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHADOWPEN]);
            Move(_rp(obj), x * data->cellwidth + offx,
                           y * data->cellheight + offy);
            Draw(_rp(obj), x * data->cellwidth + data->cellwidth - 1 + offx,
                           y * data->cellheight + offy);
            Draw(_rp(obj), x * data->cellwidth + data->cellwidth - 1 + offx,
                           y * data->cellheight + data->cellheight - 1 + offy);
            Draw(_rp(obj), x * data->cellwidth + offx,
                           y * data->cellheight + data->cellheight - 1 + offy);
            Draw(_rp(obj), x * data->cellwidth + offx,
                           y * data->cellheight + offy);

            if (y > 0)
            {
                if ((day >= 1) && (day <= mdays))
                {
                    sprintf(buf, "%d", day);
                    SetSoftStyle(_rp(obj), FS_NORMAL, AskSoftStyle(_rp(obj)));
                    SetAPen(_rp(obj), _dri(obj)->dri_Pens[day == data->clockdata.mday ? SHADOWPEN : FILLTEXTPEN]);
                    text = buf;
                }
            }
            else
            {
                SetSoftStyle(_rp(obj), FSF_BOLD, AskSoftStyle(_rp(obj)));
                SetAPen(_rp(obj), _dri(obj)->dri_Pens[SHINEPEN]);
                
                text = data->daylabels[(x + data->firstweekday) % 7];
            }
            
            if (text)
            {
                WORD tx, ty, tw;
                
                tw = TextLength(_rp(obj), text, strlen(text));
                tx = offx + x * data->cellwidth + (data->cellwidth - tw) / 2;
                ty = offy + y * data->cellheight + (data->cellheight - _font(obj)->tf_YSize) / 2;
                
                Move(_rp(obj), tx, ty + _font(obj)->tf_Baseline);
                Text(_rp(obj), text, strlen(text));
            }
            
            day++;
            
        } /* for(x = 0; x < 7; x++) */
        
    } /* for(y = 0; y < 7; y++) */
    
    return 0;
}


/*
 * Convert mouse coordinates into a day clamped to current month.
 * Return zero when the event or rendered grid is unavailable.
 */
static WORD DayUnderMouse(Object *obj, struct Calendar_DATA *data, struct IntuiMessage *imsg)
{
    WORD x, y, offx, offy, i;

    if (!imsg || data->cellwidth <= 0 || data->cellheight <= 0)
        return 0;

    offx = (_mwidth(obj) - data->cellwidth * 7 - 2) / 2 + 1;
    offy = (_mheight(obj) - data->cellheight * 7 - 1) / 2 + data->cellheight;
    
    x = imsg->MouseX - _mleft(obj) - offx;
    y = imsg->MouseY - _mtop(obj) - offy;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= data->cellwidth * 7) x = data->cellwidth * 7 - 1;
    if (y >= data->cellheight * 6) y = data->cellheight * 6 - 1;
    
    x /= data->cellwidth;
    y /= data->cellheight;
    
    i = data->firstweekday - data->mwday + 1;
    if (i > 1) i -= 7;
    
    i += y * 7 + x;
    
    if (i < 1)
    {
        i = 1;
    }
    else if (i > NumMonthDays(&data->clockdata))
    {
        i = NumMonthDays(&data->clockdata);
    }

    return i;
}


#define _between(a,x,b) ((x)>=(a) && (x)<=(b))


/*
 * Handle key selection and mouse drag within current month.
 * Ignore unavailable mouse geometry; release an active drag on mouse-up.
 * Handled keys return MUI_EventHandlerRC_Eat, mouse paths return zero.
 */
IPTR Calendar__MUIM_HandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct Calendar_DATA *data = INST_DATA(cl, obj);
        
    if (msg->muikey != MUIKEY_NONE)
    {
        UWORD day = data->clockdata.mday;
        
        switch(msg->muikey)
        {
            case MUIKEY_LEFT:
                if (day > 1)
                {
                    set(obj, MUIA_Calendar_MonthDay, day - 1);
                    return MUI_EventHandlerRC_Eat;
                }
                break;
                
            case MUIKEY_RIGHT:
                if (day < NumMonthDays(&data->clockdata))
                {
                    set(obj, MUIA_Calendar_MonthDay, day + 1);
                    return MUI_EventHandlerRC_Eat;
                }
                break;
                
            case MUIKEY_UP:
                {
                    UWORD newday = (day > 7) ? day - 7 : 1;
                    
                    if (newday != day)
                    {
                        set(obj, MUIA_Calendar_MonthDay, newday);
                        return MUI_EventHandlerRC_Eat;
                    }
                }
                break;
                
            case MUIKEY_DOWN:
                {
                    UWORD newday = (day < NumMonthDays(&data->clockdata) - 7) ?
                                   day + 7 : NumMonthDays(&data->clockdata);
                    
                    if (newday != day)
                    {
                        set(obj, MUIA_Calendar_MonthDay, newday);
                        return MUI_EventHandlerRC_Eat;
                    }
                }
                break;
                
            case MUIKEY_TOP:
            case MUIKEY_LINESTART:
                if (day != 1)
                {
                    set(obj, MUIA_Calendar_MonthDay, 1);
                    return MUI_EventHandlerRC_Eat;
                }
                break;
                
            case MUIKEY_BOTTOM:
            case MUIKEY_LINEEND:
                if (day != NumMonthDays(&data->clockdata))
                {
                    set(obj, MUIA_Calendar_MonthDay, NumMonthDays(&data->clockdata));
                    return MUI_EventHandlerRC_Eat;
                }
                break;
                
        }
    }
    else
    {
        WORD x1, y1, x2, y2, day;
        BOOL in;

        if (!msg->imsg)
            return 0;

        if (data->cellwidth <= 0 || data->cellheight <= 0)
        {
            /* Still end a drag if the grid disappeared during resizing. */
            if (msg->imsg->Class == IDCMP_MOUSEBUTTONS &&
                msg->imsg->Code == SELECTUP &&
                (data->ehn.ehn_Events & IDCMP_MOUSEMOVE))
            {
                DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                    (IPTR)&data->ehn);
                data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                    (IPTR)&data->ehn);
            }
            return 0;
        }

        x1 = _mleft(obj) + (_mwidth(obj)  - data->cellwidth  * 7 - 2) / 2 + 1;
        y1 = _mtop(obj)  + (_mheight(obj) - data->cellheight * 7 - 1) / 2 + data->cellheight;
        x2 = x1 + data->cellwidth * 7 - 1;
        y2 = y1 + data->cellheight * 6 - 1;
        in = _between(x1, msg->imsg->MouseX, x2) &&
             _between(y1, msg->imsg->MouseY, y2);

        switch(msg->imsg->Class)
        {
            case IDCMP_MOUSEBUTTONS:
                switch(msg->imsg->Code)
                {
                    case SELECTDOWN:
                        if (in)
                        {
                            day = DayUnderMouse(obj, data, msg->imsg);
                            if (day != data->clockdata.mday)
                            {
                                set(obj, MUIA_Calendar_MonthDay, day);
                            }
                            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                            data->ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
                            return 0;
                        }
                        break;
                        
                    case SELECTUP:
                        if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
                        {
                            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
                            data->ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
                            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
                            return 0;
                        }
                        break;
                }
                break;
                                
            case IDCMP_MOUSEMOVE:
                if (data->ehn.ehn_Events & IDCMP_MOUSEMOVE)
                {
                    day = DayUnderMouse(obj, data, msg->imsg);
                    if (day != data->clockdata.mday)
                    {
                        set(obj, MUIA_Calendar_MonthDay, day);
                    }
                    return 0;
                }
                break;
                
        } /* switch(msg->imsg->Class) */
        
    } /* if (msg->muikey == MUIKEY_NONE) */
    
    return 0;
}
