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

/*** Methods ****************************************************************/
IPTR Graph__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct Graph_DATA *data;

    bug("[Graph] %s()\n", __func__);

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
    }

    return (IPTR)obj;
}


IPTR Graph__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    bug("[Graph] %s()\n", __func__);

    return DoSuperMethodA(cl, obj, msg);
}


IPTR Graph__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags  = msg->ops_AttrList;
    struct TagItem   	 *tag;
    BOOL    	      	  redraw = FALSE;

    bug("[Graph] %s()\n", __func__);

    while ((tag = NextTagItem(&tags)) != NULL)
    {
    	switch(tag->ti_Tag)
	{
    	    case MUIA_Graph_InfoText:
                FreeVec(data->graph_InfoText);
		data->graph_InfoText = StrDup((char *)tag->ti_Data);
		redraw = TRUE;
		break;
	}
    }

    if (redraw)
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Graph__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
//    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR    	      retval = TRUE;

    bug("[Graph] %s()\n", __func__);

    switch(msg->opg_AttrID)
    {
    	default:
	    retval = DoSuperMethodA(cl, obj, (Msg)msg);
	    break;
    }
    
    return retval;
}


IPTR Graph__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    bug("[Graph] %s()\n", __func__);

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->ihn);
    
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

    return TRUE;
}


IPTR Graph__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
 
    bug("[Graph] %s()\n", __func__);

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

    DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->ihn);
    
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
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct Region   	 *region;
    struct Rectangle	 rect;
    APTR    	    	 clip = NULL;
    int                 offset;

    bug("[Graph] %s()\n", __func__);

    region = NewRegion();
    if (region)
    {
    	rect.MinX = _left(obj);
	rect.MinY = _top(obj);
	rect.MaxX = _right(obj);
	rect.MaxY = _bottom(obj);
	
	OrRectRegion(region, &rect);

	clip = MUI_AddClipRegion(muiRenderInfo(obj), region);
    }
    
    DoSuperMethodA(cl, obj, (Msg)msg);

    /* Render our graph.. */
    if ((msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
    {
        // First fille the background ..
        SetAPen(_rp(obj), data->graph_BackPen);
        RectFill(_rp(obj), rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

        // Draw the segment divisions..
        SetAPen(_rp(obj), data->graph_SegmentPen);
        for (offset = rect.MinX; offset <= rect.MaxX; offset += data->graph_SegmentSize)
        {
            Move(_rp(obj), offset, rect.MinY);
            Draw(_rp(obj), offset, rect.MaxY);
        }
        for (offset = rect.MaxY; offset >= rect.MinY; offset -= data->graph_SegmentSize)
        {
            Move(_rp(obj), rect.MinX, offset);
            Draw(_rp(obj), rect.MaxX, offset);
        }

        // Draw the Axis..
        SetAPen(_rp(obj), data->graph_AxisPen);
        Move(_rp(obj), rect.MinX, rect.MinY);
        Draw(_rp(obj), rect.MaxX, rect.MinY);
        Draw(_rp(obj), rect.MaxX, rect.MaxY);
        Draw(_rp(obj), rect.MinX, rect.MaxY);
        Draw(_rp(obj), rect.MinX, rect.MinY);

#if (0)
        // Add the InfoText
        if (data->graph_InfoText)
        {
            UWORD text_wid = TextLength(_rp(obj), data->graph_InfoText, strlen(data->graph_InfoText));
            if (text_wid > 0)
            {
                SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
                Move(_rp(obj), ((rect.MinX + rect.MaxX) /2) + (text_wid / 2), ((rect.MinY + rect.MaxY) /2) - (_font(obj)->tf_YSize / 2));
                Text(_rp(obj), (CONST_STRPTR)data->graph_InfoText, text_wid);
            }
        }
#endif
    }
    if (region)
    {
    	MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    }

    return 0;
}

IPTR Graph__MUIM_Graph_Timer(Class *cl, Object *obj, Msg msg)
{
    struct Graph_DATA *data;

    bug("[Graph] %s()\n", __func__);

    data = INST_DATA(cl, obj);
    
    if (!data->graph_Flags & GRAPHF_PERIODIC)
    {
	set(obj, MUIA_Graph_PeriodicTick, TRUE);
    }
    
    return 0;
}
