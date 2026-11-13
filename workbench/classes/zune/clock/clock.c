/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/date.h>

#include <aros/debug.h>
#include <aros/asmcall.h>

#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "clock.h"
#include "clock_private.h"

#define SHADOW_OFFX 4
#define SHADOW_OFFY 4


/****** Clock.mcc/MUIC_Clock ************
*
*   NAME
*       MUIC_Clock
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Analog clock, initialized from MUIA_Clock_Time or the system clock.
*       An Application timer refreshes Time from the system clock once per
*       delivered tick unless Frozen; it does not increment a custom time.
*
*   NOTES
*       Time is copied on input and borrowed on Get. Setup/Cleanup own the
*       timer handler; Show/Hide own the highlight pen.
*
*   BUGS
*       Frozen does not redraw or remove the timer. Individual time tags
*       are not processed during construction. Ticked is notification-only.
*       Time notifications during a timer call may contain a stack pointer;
*       copy it synchronously or query Time. Failed graphics allocation skips drawing.
*
*/
/****** Clock.mcc/MUIA_Clock_Time ************
*
*   NAME
*       MUIA_Clock_Time
*
*   FUNCTION
*       Get or replace the displayed ClockData.
*
*   INPUTS
*       Initialize, set, get; non-NULL ClockData pointer for input.
*
*   NOTES
*       Input is copied; Get returns a pointer borrowed until object disposal,
*       with contents subject to updates. No input validation. A non-frozen
*       timer refresh replaces it. Set redraws but does not emit Ticked or
*       individual component notifications. Timer notifications can carry a
*       temporary input pointer; do not retain it.
*
*/
/****** Clock.mcc/MUIA_Clock_Hour ************
*
*   NAME
*       MUIA_Clock_Hour
*
*   FUNCTION
*       Get or set the hour in ClockData.
*
*   INPUTS
*       Set, get; UWORD, 0..23. Not parsed during construction.
*
*   NOTES
*       No range validation or carrying; redraws and may notify this tag.
*       No Time/Ticked notification. A non-frozen tick overwrites the value.
*
*/
/****** Clock.mcc/MUIA_Clock_Min ************
*
*   NAME
*       MUIA_Clock_Min
*
*   FUNCTION
*       Get or set minutes in ClockData.
*
*   INPUTS
*       Set, get; UWORD, 0..59. Not parsed during construction.
*
*   NOTES
*       No range validation or carrying; redraws and may notify this tag.
*       No Time/Ticked notification. A non-frozen tick overwrites the value.
*
*/
/****** Clock.mcc/MUIA_Clock_Sec ************
*
*   NAME
*       MUIA_Clock_Sec
*
*   FUNCTION
*       Get or set seconds in ClockData.
*
*   INPUTS
*       Set, get; UWORD, 0..59. Not parsed during construction.
*
*   NOTES
*       No range validation or carrying; redraws and may notify this tag.
*       No Time/Ticked notification. A non-frozen tick overwrites the value.
*
*/
/****** Clock.mcc/MUIA_Clock_Frozen ************
*
*   NAME
*       MUIA_Clock_Frozen
*
*   FUNCTION
*       Suspend timer-driven Time refreshes.
*
*   INPUTS
*       Initialize, set, get; BOOL, default FALSE.
*
*   NOTES
*       The timer remains registered. Explicit Time and component setters
*       still work. Changing Frozen alone does not redraw.
*
*/
/****** Clock.mcc/MUIA_Clock_EditHand ************
*
*   NAME
*       MUIA_Clock_EditHand
*
*   FUNCTION
*       Select the hand highlighted while Frozen.
*
*   INPUTS
*       Initialize, set, get; WORD, default -1; 0=hour, 1=minute,
*       2=second (MUIV_Clock_EditHand_*).
*
*   NOTES
*       Other values highlight nothing. Set redraws; this is not an
*       interactive editing facility.
*
*/
/****** Clock.mcc/MUIA_Clock_Ticked ************
*
*   NAME
*       MUIA_Clock_Ticked
*
*   FUNCTION
*       Notify listeners after a non-frozen timer refresh.
*
*   INPUTS
*       Notification tag TRUE; no stored setter/getter value.
*
*   NOTES
*       Emitted after Time on each delivered non-frozen timer call, even
*       when time is unchanged. Direct setters do not emit Ticked. Use
*       EveryTime notifications when every delivery matters.
*
*/
/****** Clock.mcc/MUIM_Clock_Timer ************
*
*   NAME
*       MUIM_Clock_Timer
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Clock_Timer);
*
*   FUNCTION
*       Refresh Time from the system clock unless Frozen.
*
*   INPUTS
*       Live Clock object; no payload.
*
*   RESULT
*       Always zero; no error status.
*
*   NOTES
*       Sets Time followed by Ticked=TRUE. The Time notification may
*       expose temporary ClockData; do not retain that pointer. This method
*       does not enforce a one-second interval or advance custom time.
*
*/

/*** Methods ****************************************************************/
/*
 * Copy Time or obtain system time; initialize Frozen/EditHand and the
 * deferred Application timer. Individual time components are ignored
 * at construction. Return object or zero on superclass failure.
 */
IPTR Clock__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct Clock_DATA *data;
    struct TagItem      *ti;
    
    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
    
        MUIA_InnerLeft,   4,
        MUIA_InnerTop,    4,
        MUIA_InnerRight,  4,
        MUIA_InnerBottom, 4,
        
        TAG_MORE, (IPTR) msg->ops_AttrList
    );
        
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


/*
 * Delegate disposal; callers must complete Hide/Cleanup and release
 * borrowed ClockData pointers beforehand.
 */
IPTR Clock__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Apply Time/component flags and request redraw except for Frozen-only
 * changes; delegate tags unmodified. Requires valid Time pointer.
 */
IPTR Clock__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Clock_DATA *data = INST_DATA(cl, obj);
    struct TagItem       *tags  = msg->ops_AttrList;
    struct TagItem       *tag;
    BOOL                  redraw = FALSE;
    
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


/*
 * Return fields or borrowed ClockData through writable storage.
 * Ticked has no getter; otherwise delegate unknown attributes.
 */
IPTR Clock__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Clock_DATA *data = INST_DATA(cl, obj);
    IPTR              retval = TRUE;
    
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


/*
 * Register the timer handler after successful Area setup. Failure
 * to register is not reported separately; Cleanup removes the node.
 */
IPTR Clock__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Clock_DATA *data = INST_DATA(cl, obj);
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);

    return TRUE;
}

/*
 * Acquire highlight pen after successful Show; failed pen allocation
 * falls back to shine. Hide releases the acquired pen.
 */
IPTR Clock__MUIM_Show(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Clock_DATA *data = INST_DATA(cl, obj);
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    data->editpen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                                  0xFFFFFFFF,
                                  0xD8D8D8D8,
                                  0x00000000,
                                  OBP_Precision, PRECISION_GUI,
                                  OBP_FailIfBad, FALSE,
                                  TAG_DONE);
    return TRUE;
}

/*
 * Release highlight pen, then delegate Hide. Bitmap survives until
 * Cleanup.
 */
IPTR Clock__MUIM_Hide(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Clock_DATA *data = INST_DATA(cl, obj);
 
    if (data->editpen != -1)
    {
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->editpen);
        data->editpen = -1;
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/*
 * Release rendering buffers and Application timer handler, then
 * delegate Cleanup. Requires no active timer/draw callbacks.
 */
IPTR Clock__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Clock_DATA *data = INST_DATA(cl, obj);
 
    if (data->clockbm)
    {
        FreeBitMap(data->clockbm);
        FreeRaster(data->clockraster, data->clockbmw, data->clockbmh);
        data->clockbm = NULL;
        data->clockraster = NULL;
    }
    DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->ihn);
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/*
 * Add 31 to minimum/default Area dimensions; return TRUE.
 */
IPTR Clock__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
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


#define MY_PI 3.14159265358979323846

/*
 * Draw a filled hand on an initialized RastPort with AreaInfo/TmpRas.
 * Requires valid geometry and representable WORD coordinates.
 */
static void DrawHand
(
    struct RastPort *rp, WORD cx, WORD cy, DOUBLE angle,
    WORD radius1, WORD radius2
)
{
    WORD x, y;
    
    x = cx + (WORD)(cos(angle + MY_PI) * radius1);
    y = cy - (WORD)(sin(angle + MY_PI) * radius1);
    
    AreaMove(rp, x, y);

    x = cx + (WORD)(cos(angle + MY_PI / 2.0) * radius1);
    y = cy - (WORD)(sin(angle + MY_PI / 2.0) * radius1);
    
    AreaDraw(rp, x, y);
    
    x = cx + (WORD)(cos(angle) * radius2);
    y = cy - (WORD)(sin(angle) * radius2);
    
    AreaDraw(rp, x, y);
    
    x = cx + (WORD)(cos(angle - MY_PI / 2.0) * radius1);
    y = cy - (WORD)(sin(angle - MY_PI / 2.0) * radius1);
     
    AreaDraw(rp, x, y);
    
    x = cx + (WORD)(cos(angle + MY_PI) * radius1);
    y = cy - (WORD)(sin(angle + MY_PI) * radius1);
    
    AreaDraw(rp, x, y);
    
    AreaEnd(rp);
}


/*
 * Draw a line hand; geometry must fit WORD coordinates.
 */
static void DrawThinHand(struct RastPort *rp, WORD cx, WORD cy, DOUBLE angle, WORD radius)
{
    WORD x, y;
    
    Move(rp, cx, cy);
    
    x = cx + (WORD)(cos(angle) * radius);
    y = cy - (WORD)(sin(angle) * radius);
    
    Draw(rp, x, y);
}

/*
 * Draw using cached/offscreen bitmap and temporary raster. Requires
 * valid graphics lifecycle; allocation failure skips drawing. Returns 0.
 */
IPTR Clock__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Clock_DATA *data = INST_DATA(cl, obj);
    struct RastPort      *obj_rp;
    struct Region        *region;
    struct Rectangle     rect;
    APTR                 clip = NULL;
    WORD                 r, x, y, x2, y2, i, cx, cy, c;
    WORD                 new_clockbmw, new_clockbmh;
    WORD                 clock_posx, clock_posy;
    DOUBLE               angle;

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
            x = cx + (WORD)(cos(angle) * r);
            y = cy - (WORD)(sin(angle) * r);

            if ((i % 5) == 0)
            {
                x2 = cx + (WORD)(cos(angle) * (r * 90 / 100));
                y2 = cy - (WORD)(sin(angle) * (r * 90 / 100));
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
            if ((data->edithand == MUIV_Clock_EditHand_Hour) && data->frozen)
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
            if ((data->edithand == MUIV_Clock_EditHand_Minute) && data->frozen)
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
            if ((data->edithand == MUIV_Clock_EditHand_Second) && data->frozen)
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


/*
 * Unless frozen, set Time from system time, then emit Ticked.
 * Time notification uses temporary ClockData; callbacks must copy it.
 * Always returns zero.
 */
IPTR Clock__MUIM_Clock_Timer(Class *cl, Object *obj, Msg msg)
{
    struct Clock_DATA *data;
    struct ClockData      cd;
    struct timeval        tv;
    
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
