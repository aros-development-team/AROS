/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <utility/date.h>

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

#include "graph.h"
#include "graph_intern.h"

IPTR Graph__UpdateSourceArray(struct Graph_DATA *data, IPTR count)
{
    struct Graph_SourceDATA *newSourceArray = data->graph_Sources;

    D(bug("[Graph] %s(%d)\n", __func__, count);)

    if (data->graph_SourceCount != count)
    {
        IPTR copycnt;

        if (count > data->graph_SourceCount)
            copycnt = data->graph_SourceCount;
        else
            copycnt = count;

        newSourceArray = AllocMem(sizeof(struct Graph_SourceDATA) * count, MEMF_ANY);
        if (newSourceArray)
        {

            if (data->graph_Sources)
            {
                CopyMem(data->graph_Sources, newSourceArray, sizeof(struct Graph_SourceDATA) * copycnt);
                FreeMem(data->graph_Sources, sizeof(struct Graph_SourceDATA) * data->graph_SourceCount);
            }

            if (count > data->graph_SourceCount)
            {
                D(bug("[Graph] %s: initializing new source\n", __func__);)
                memset(&newSourceArray[count - 1], 0, sizeof(struct Graph_SourceDATA)); 
                newSourceArray[count - 1].gs_PlotPen = -1;
                newSourceArray[count - 1].gs_PlotFillPen = -1;
                if (data->graph_EntryCount > 0)
                    newSourceArray[count - 1].gs_Entries = AllocMem(sizeof(IPTR) * data->graph_EntryCount, MEMF_CLEAR|MEMF_ANY);
            }

            data->graph_Sources = newSourceArray;
            data->graph_SourceCount = count;
        }
    }

    return (IPTR)newSourceArray;
}

IPTR Graph__UpdateSourceEntries(struct Graph_DATA *data, IPTR sourceNo, IPTR count)
{
    struct Graph_SourceDATA *dataSource = NULL;

    D(bug("[Graph] %s(%d:%d)\n", __func__, sourceNo, count);)

    if (count > data->graph_EntryCount)
    {
        IPTR *newEntries;

        dataSource = &data->graph_Sources[sourceNo];

        newEntries =  AllocMem(sizeof(IPTR) * count, MEMF_ANY);
        if (newEntries)
        {
            if (dataSource->gs_Entries)
            {
                CopyMem(dataSource->gs_Entries, newEntries, sizeof(IPTR) * data->graph_EntryCount);
                FreeMem(dataSource->gs_Entries, sizeof(IPTR) * data->graph_EntryCount);
            }
            dataSource->gs_Entries = newEntries;
        }
    }

    return (IPTR)dataSource;
}

/*** Methods ****************************************************************/
IPTR Graph__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct Graph_DATA *data;

    D(bug("[Graph] %s()\n", __func__);)

    obj = (Object *) DoSuperNewTags
    (
        cl, obj, NULL,
    
        MUIA_InnerLeft,   4,
	MUIA_InnerTop,    4,
	MUIA_InnerRight,  4,
	MUIA_InnerBottom, 4,
	
        TAG_MORE, (IPTR) msg->ops_AttrList	
    );

    if (obj)
    {
        data = INST_DATA(cl, obj);

        data->graph_Flags = 0;
        data->graph_BackPen = -1;
        data->graph_AxisPen = -1;
        data->graph_SegmentPen = -1;

        /* default segment size ... */
         data->graph_SegmentSize = 10;

        /* We always have atleast one source .. */
        Graph__UpdateSourceArray(data, 1);

        data->ihn.ihn_Flags  = MUIIHNF_TIMER;
        data->ihn.ihn_Method = MUIM_Graph_Timer;
        data->ihn.ihn_Object = obj;
        data->ihn.ihn_Millis = 1000;

        SetAttrsA(obj, msg->ops_AttrList);
    }

    return (IPTR)obj;
}

IPTR Graph__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    int i;

    D(bug("[Graph] %s()\n", __func__);)

    if (data->graph_SourceCount > 0)
    {
        if (data->graph_EntryCount > 0)
        {
            for (i = 0; i < data->graph_SourceCount; i ++)
            {
                FreeMem(data->graph_Sources[i].gs_Entries, sizeof(IPTR) * data->graph_EntryCount);
            }
        }
        FreeMem(data->graph_Sources, sizeof(struct Graph_SourceDATA) * data->graph_SourceCount);
    }

    return DoSuperMethodA(cl, obj, msg);
}


IPTR Graph__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags  = msg->ops_AttrList;
    struct TagItem   	 *tag;
    BOOL    	      	  redraw = FALSE;

    D(bug("[Graph] %s()\n", __func__);)

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case MUIA_Graph_Max:
                data->graph_Max = tag->ti_Data;
                break;

            case MUIA_Graph_InfoText:
                FreeVec(data->graph_InfoText);
                data->graph_InfoText = StrDup((char *)tag->ti_Data);
                redraw = TRUE;
                break;

            case MUIA_Graph_EntryCount:
                if (tag->ti_Data)
                {
                    int i;
                    for (i = 0; i < data->graph_SourceCount; i ++)
                    {
                        Graph__UpdateSourceEntries(data, i, tag->ti_Data);
                    }
                    data->graph_EntryCount = tag->ti_Data;
                    data->graph_Flags |= GRAPHF_FIXEDLEN;
                }
                else
                {
                    data->graph_Flags &= ~GRAPHF_FIXEDLEN;
                }
                break;

            case MUIA_Graph_UpdateInterval:
                if (tag->ti_Data)
                {
                    data->graph_Flags |= GRAPHF_PERIODIC;
                    data->ihn.ihn_Millis = tag->ti_Data;
                    if ((data->graph_Flags & GRAPHF_SETUP) && !(data->graph_Flags & GRAPHF_HANDLER))
                    {
                        data->graph_Flags |= GRAPHF_HANDLER;
                        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);
                    }
                }
                else
                {
                    data->graph_Flags &= ~GRAPHF_PERIODIC;
                    if ((data->graph_Flags & GRAPHF_SETUP) && (data->graph_Flags & GRAPHF_HANDLER))
                    {
                        DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->ihn);
                        data->graph_Flags &= ~GRAPHF_HANDLER;
                    }
                }
                break;
	}
    }

    if (redraw)
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Graph__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR    	      retval = TRUE;

    D(bug("[Graph] %s()\n", __func__);)

    switch(msg->opg_AttrID)
    {
        case MUIA_Graph_Max:
            *(msg->opg_Storage) = data->graph_Max;
            break;

        case MUIA_Graph_EntryCount:
            *(msg->opg_Storage) = data->graph_EntryCount;
            break;

        case MUIA_Graph_UpdateInterval:
            *(msg->opg_Storage) = data->ihn.ihn_Millis;
            break;

    	default:
	    retval = DoSuperMethodA(cl, obj, (Msg)msg);
	    break;
    }
    
    return retval;
}

IPTR Graph__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    D(bug("[Graph] %s()\n", __func__);)

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    if ((data->graph_Flags & GRAPHF_PERIODIC) && !(data->graph_Flags & GRAPHF_HANDLER))
    {
        data->graph_Flags |= GRAPHF_HANDLER;
        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);
    }

    data->graph_BackPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
				  0xF2F2F2F2,
				  0xF8F8F8F8,
				  0xFAFAFAFA,
				  OBP_Precision, PRECISION_GUI,
				  OBP_FailIfBad, FALSE,
				  TAG_DONE);

    data->graph_AxisPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
    	    	    	    	  0x7A7A7A7A,
				  0xC5C5C5C5,
				  0xDEDEDEDE,
				  OBP_Precision, PRECISION_GUI,
				  OBP_FailIfBad, FALSE,
				  TAG_DONE);

    data->graph_SegmentPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
    	    	    	    	  0x85858585,
				  0xD3D3D3D3,
				  0xEDEDEDED,
				  OBP_Precision, PRECISION_GUI,
				  OBP_FailIfBad, FALSE,
				  TAG_DONE);

    data->graph_Flags |= GRAPHF_SETUP;

    return TRUE;
}


IPTR Graph__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
 
    D(bug("[Graph] %s()\n", __func__);)

    data->graph_Flags &= ~GRAPHF_SETUP;

    if (data->graph_SegmentPen != -1)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_SegmentPen);
    	data->graph_SegmentPen = -1;
    }

    if (data->graph_AxisPen != -1)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_AxisPen);
    	data->graph_AxisPen = -1;
    }

    if (data->graph_BackPen != -1)
    {
    	ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_BackPen);
    	data->graph_BackPen = -1;
    }

    if ((data->graph_Flags & GRAPHF_PERIODIC) && (data->graph_Flags & GRAPHF_HANDLER))
    {
        DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->ihn);
        data->graph_Flags &= ~GRAPHF_HANDLER;
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Graph__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    UWORD nominalsize = (data->graph_SegmentSize * 10);

    bug("[Graph] %s()\n", __func__);

    DoSuperMethodA(cl, obj, (Msg)msg);
    
    msg->MinMaxInfo->MinWidth  += nominalsize;
    msg->MinMaxInfo->MinHeight += nominalsize;
    msg->MinMaxInfo->DefWidth  += nominalsize;
    msg->MinMaxInfo->DefHeight += nominalsize;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;

    return TRUE;
}

IPTR Graph__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Graph_DATA           *data = INST_DATA(cl, obj);
    struct Graph_SourceDATA     *sourceData;
    struct Region   	        *region;
    struct Rectangle	        rect;
    APTR    	    	        clip = NULL;
    UWORD                       pos, offset = 0, src;
    UWORD                       objHeight;

    D(bug("[Graph] %s()\n", __func__);)

    if (data->graph_Flags & GRAPHF_FIXEDLEN)
        data->graph_SegmentSize = (_right(obj) - _left(obj) )/ data->graph_EntryCount;

    rect.MinX = _left(obj);
    rect.MinY = _top(obj);
    rect.MaxX = _right(obj);
    rect.MaxY = _bottom(obj);

    objHeight = rect.MaxY - rect.MinY;

    D(
        bug("[Graph] %s: height %d, segemnt size %d\n", __func__, objHeight, data->graph_SegmentSize);
        bug("[Graph] %s: tick %d\n", __func__, data->graph_Tick);
    )
    
    region = NewRegion();
    if (region)
    {
	OrRectRegion(region, &rect);

	clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
    }
    
    DoSuperMethodA(cl, obj, (Msg)msg);

    /* Render our graph.. */
    if ((msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
    {
        if ((data->graph_Flags & GRAPHF_PERIODIC) && (data->graph_Flags & GRAPHF_FIXEDLEN))
            offset = data->graph_Tick;

        // First fille the background ..
        SetAPen(_rp(obj), data->graph_BackPen);
        RectFill(_rp(obj), rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

        // Draw the segment divisions..
        SetAPen(_rp(obj), data->graph_SegmentPen);
        for (pos = rect.MinX; pos <= (rect.MaxX + data->graph_SegmentSize); pos += data->graph_SegmentSize)
        {
            Move(_rp(obj), pos - offset, rect.MinY);
            Draw(_rp(obj), pos - offset, rect.MaxY);
        }
        for (pos = rect.MaxY; pos >= rect.MinY; pos -= data->graph_SegmentSize)
        {
            Move(_rp(obj), rect.MinX, pos);
            Draw(_rp(obj), rect.MaxX, pos);
        }

        // Draw the Axis..
        SetAPen(_rp(obj), data->graph_AxisPen);
        Move(_rp(obj), rect.MinX, rect.MinY);
        Draw(_rp(obj), rect.MaxX, rect.MinY);
        Draw(_rp(obj), rect.MaxX, rect.MaxY);
        Draw(_rp(obj), rect.MinX, rect.MaxY);
        Draw(_rp(obj), rect.MinX, rect.MinY);

        // Plot the entries..
        if (data->graph_Sources) 
        {
            for (src = 0; src < data->graph_SourceCount; src ++)
            {
                sourceData = &data->graph_Sources[src];

                SetAPen(_rp(obj), data->graph_Sources[src].gs_PlotPen);
                Move(_rp(obj), rect.MinX - offset, rect.MaxY);

                for (pos = 1; pos < data->graph_EntryPtr; pos++)
                {
                    UWORD ypos = (objHeight / data->graph_Max) * sourceData->gs_Entries[pos];
                    Draw(_rp(obj),
                        rect.MinX + (pos * data->graph_SegmentSize) - offset,
                        rect.MaxY - ypos);
                }
            }
        }

        // Add the InfoText
        if (data->graph_InfoText)
        {
            UWORD text_wid = TextLength(_rp(obj), data->graph_InfoText, strlen(data->graph_InfoText));

            D(bug("[Graph] %s: strlen = %d, wid = %d\n", __func__, strlen(data->graph_InfoText), text_wid);)

            if (text_wid > 0)
            {
                SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
                Move(_rp(obj), ((rect.MinX + rect.MaxX) /2) - (text_wid / 2), ((rect.MinY + rect.MaxY) /2) + (_font(obj)->tf_YSize / 2));
                Text(_rp(obj), (CONST_STRPTR)data->graph_InfoText, text_wid);
            }
        }
    }
    if (region)
    {
    	MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    }

    D(bug("[Graph] %s: done\n", __func__);)

    return 0;
}

IPTR Graph__MUIM_Graph_GetSourceHandle(Class *cl, Object *obj, struct MUIP_Graph_GetSourceHandle *msg)
{
    struct Graph_DATA *data;
    IPTR retVal = 0;

    D(bug("[Graph] %s(%d)\n", __func__, msg->SourceNo);)

    data = INST_DATA(cl, obj);
    if (msg->SourceNo >= data->graph_SourceCount)
        Graph__UpdateSourceArray(data, (msg->SourceNo + 1));

    retVal = (IPTR)&data->graph_Sources[msg->SourceNo];

    return retVal;
}

IPTR Graph__MUIM_Graph_SetSourceAttrib(Class *cl, Object *obj, struct MUIP_Graph_SetSourceAttrib *msg)
{
    struct Graph_SourceDATA *dataSource = (struct Graph_SourceDATA *)msg->SourceHandle;

    D(bug("[Graph] %s()\n", __func__);)

    switch (msg->Attrib)
    {
        case MUIV_Graph_Source_ReadHook:
            dataSource->gs_ReadHook = (struct Hook *)msg->AttribVal;
            break;
        case MUIV_Graph_Source_Pen:
            dataSource->gs_PlotPen = (WORD)msg->AttribVal;
            break;
        case MUIV_Graph_Source_FillPen:
            dataSource->gs_PlotFillPen = (WORD)msg->AttribVal;
            break;
    }

    return 0;
}

IPTR Graph__MUIM_Graph_Timer(Class *cl, Object *obj, Msg msg)
{
    struct Graph_DATA *data;
    int i;

    D(bug("[Graph] %s()\n", __func__);)

    data = INST_DATA(cl, obj);
    
    if (data->graph_Tick++ == data->graph_SegmentSize)
        data->graph_Tick = 0;

    if (data->graph_Flags & GRAPHF_PERIODIC)
    {
        if (data->graph_SourceCount > 0)
        {
            BOOL updateEntries = FALSE, updated = FALSE;

            if (data->graph_Flags & GRAPHF_FIXEDLEN) 
            {
                if (data->graph_EntryPtr >= data->graph_EntryCount)
                    data->graph_EntryPtr = data->graph_EntryCount - 1;
            }
            else
            {
                if (!(data->graph_EntryCount) || (data->graph_EntryPtr >= data->graph_EntryCount))
                    updateEntries = TRUE;
            }

            D(bug("[Graph] %s: reading entry %d\n", __func__, data->graph_EntryPtr);)

            for (i = 0; i < data->graph_SourceCount; i ++)
            {
                if (data->graph_Sources[i].gs_ReadHook)
                {
                    if (updateEntries)
                    {
                        Graph__UpdateSourceEntries(data, i, (data->graph_EntryPtr + 1));
                        updated = TRUE;
                    }

                    D(bug("[Graph] %s: source %d entries @ 0x%p\n", __func__, i, data->graph_Sources[i].gs_Entries);)
                    CALLHOOKPKT(data->graph_Sources[i].gs_ReadHook,
                        (APTR)&data->graph_Sources[i].gs_Entries[data->graph_EntryPtr],
                        data->graph_Sources[i].gs_ReadHook->h_Data);
                }
            }
            if (updated)
                data->graph_EntryCount++;
            data->graph_EntryPtr++;
        }

	SET(obj, MUIA_Graph_PeriodicTick, TRUE);
    }
    
    return 0;
}
