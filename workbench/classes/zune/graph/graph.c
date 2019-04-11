/*
    Copyright © 2017-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
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

void Graph__FreeInfoText(Class *cl, Object *obj)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct Node *infoLine, *tmp;

    D(bug("[Graph] %s()\n", __func__);)

    ForeachNodeSafe(&data->graph_InfoText, infoLine, tmp)
    {
        D(bug("[Graph] %s: Deleting old infotext line @ 0x%p - '%s'\n", __func__, infoLine, infoLine->ln_Name);)
        Remove(infoLine);
        FreeVec(infoLine);
    }
    data->graph_ITHeight = 0;
}

IPTR Graph__ParseInfoText(Class *cl, Object *obj, char *infoTxt)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    int i, srcLen, start = 0;
    struct Node *infoLine;

    D(bug("[Graph] %s(0x%p)\n", __func__, infoTxt);)

    Graph__FreeInfoText(cl, obj);

    if (infoTxt)
    {
        srcLen = strlen(infoTxt);

        for (i = 0; i < (srcLen + 1); i ++)
        {
            if (((i - start) > 0) && (infoTxt[i] == '\n') || (infoTxt[i] == '\0'))
            {
                infoLine = (struct Node *)AllocVec(sizeof(struct Node) + 1 + (i - start), MEMF_ANY|MEMF_CLEAR);
                infoLine->ln_Name = (char *)&infoLine[1];

                CopyMem(&infoTxt[start], infoLine->ln_Name, (i - start));

                D(bug("[Graph] %s: New infotext line @ 0x%p - '%s'\n", __func__, infoLine, infoLine->ln_Name);)
                AddTail(&data->graph_InfoText, infoLine);

                data->graph_ITHeight += 1;
                start = i + 1;
            }
        }
    }
    D(bug("[Graph] %s: InfoText>  %d lines\n", __func__, data->graph_ITHeight);)
    return data->graph_ITHeight;
}

IPTR Graph__SumValues(IPTR *start, int count)
{
    IPTR retVal = 0;
    int i;
    for (i = 0; i < count; i++)
    {
        retVal += start[i];
    }
    if (retVal > 0)
        retVal /= count;
    return retVal;
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

        NEWLIST(&data->graph_InfoText);
        data->graph_ITHeight = 0;

        data->graph_RastPort = NULL;

        data->graph_Flags =  (GRAPHF_DRAWAXIS|GRAPHF_DRAWSEGS);
        data->graph_BackPen = -1;
        data->graph_AxisPen = -1;
        data->graph_SegmentPen = -1;

        /* We always have atleast one source .. */
        Graph__UpdateSourceArray(data, 1);

        data->ihn.ihn_Flags  = MUIIHNF_TIMER;
        data->ihn.ihn_Method = MUIM_Graph_Timer;
        data->ihn.ihn_Object = obj;
        data->ihn.ihn_Millis = 1000;

        data->graph_PeriodCeiling = data->ihn.ihn_Millis * 10;
        data->graph_PeriodStepping = data->ihn.ihn_Millis;

        data->graph_ValCeiling = 100;
        data->graph_ValStepping = 10;

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

    Graph__FreeInfoText(cl, obj);

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
            /* Aggreagte mode plots the sum of source entries/no of sources */
            case MUIA_Graph_Aggregate:
                data->graph_Flags &= ~GRAPHF_AGGR;
                if (tag->ti_Data)
                    data->graph_Flags |= GRAPHF_AGGR;
                break;

            /* Set the info text to display */
            case MUIA_Graph_InfoText:
                Graph__ParseInfoText(cl, obj, (char *)tag->ti_Data);
                redraw = TRUE;
                break;

            /* Set the input value ceiling and stepping */
            case MUIA_Graph_ValueCeiling:
                data->graph_ValCeiling = tag->ti_Data;
                break;

            case MUIA_Graph_ValueStep:
                data->graph_ValStepping = tag->ti_Data;
                break;

            /* Set the period ceiling and stepping */
            case MUIA_Graph_PeriodCeiling:
                data->graph_PeriodCeiling = tag->ti_Data;
                break;

            case MUIA_Graph_PeriodStep:
                data->graph_PeriodStepping = tag->ti_Data;
                break;

            /* Set or turn off periodic update mode */
            case MUIA_Graph_PeriodInterval:
                if (tag->ti_Data)
                {
                    data->graph_Flags |= GRAPHF_PERIODIC;
                    data->ihn.ihn_Millis = tag->ti_Data;
                    SET(obj, MUIA_Graph_EntryCount, (data->graph_PeriodCeiling/ data->ihn.ihn_Millis));
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

            /* Set or turn off Fixed entry count mode */
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
        case MUIA_Graph_ValueCeiling:
            *(msg->opg_Storage) = data->graph_ValCeiling;
            break;

        case MUIA_Graph_EntryCount:
            *(msg->opg_Storage) = data->graph_EntryCount;
            break;

        case MUIA_Graph_PeriodInterval:
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

    data->graph_Flags |= GRAPHF_SETUP;

    return TRUE;
}


IPTR Graph__MUIM_Show(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    int i;

    D(bug("[Graph] %s()\n", __func__);)

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

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

    for (i = 0; i < data->graph_SourceCount; i ++)
    {
        ULONG *penSrc;
        if (((penSrc = (ULONG *)data->graph_Sources[i].gs_PlotFillPenSrc) != NULL) &&
            (data->graph_Sources[i].gs_PlotFillPenSrc != data->graph_Sources[i].gs_PlotPenSrc))
            data->graph_Sources[i].gs_PlotFillPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
    	    	    	    	  penSrc[0],
				  penSrc[1],
				  penSrc[2],
				  OBP_Precision, PRECISION_GUI,
				  OBP_FailIfBad, FALSE,
				  TAG_DONE);

        if ((penSrc = (ULONG *)data->graph_Sources[i].gs_PlotPenSrc) != NULL)
            data->graph_Sources[i].gs_PlotPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
    	    	    	    	  penSrc[0],
				  penSrc[1],
				  penSrc[2],
				  OBP_Precision, PRECISION_GUI,
				  OBP_FailIfBad, FALSE,
				  TAG_DONE);
    }

    return TRUE;
}

IPTR Graph__MUIM_Hide(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    int i;

    D(bug("[Graph] %s()\n", __func__);)

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

    for (i = 0; i < data->graph_SourceCount; i ++)
    {
        if ((data->graph_Sources[i].gs_PlotFillPen != -1) &&
            (data->graph_Sources[i].gs_PlotFillPen != data->graph_Sources[i].gs_PlotPen))
                ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_Sources[i].gs_PlotFillPen);
        if (data->graph_Sources[i].gs_PlotPen != -1)
                ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_Sources[i].gs_PlotPen);
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR Graph__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    D(bug("[Graph] %s()\n", __func__);)

    data->graph_Flags &= ~GRAPHF_SETUP;

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

    D(bug("[Graph] %s()\n", __func__);)

    DoSuperMethodA(cl, obj, (Msg)msg);
    
    msg->MinMaxInfo->MinWidth  += (data->graph_PeriodCeiling / data->graph_PeriodStepping) * 2;
    msg->MinMaxInfo->MinHeight += (data->graph_ValCeiling / data->graph_ValStepping) * 2;
    msg->MinMaxInfo->DefWidth  += (data->graph_PeriodCeiling / data->graph_PeriodStepping) * 10;
    msg->MinMaxInfo->DefHeight += (data->graph_ValCeiling / data->graph_ValStepping) * 10;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;

    return TRUE;
}

IPTR Graph__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Graph_DATA           *data = INST_DATA(cl, obj);
    struct Graph_SourceDATA     *sourceData;
    struct Region   	        *region;
    struct Node                 *infoLine;
    struct RastPort             *renderPort;
    struct Rectangle	        rect;
    APTR    	    	        clip = NULL;
    UWORD                       pos, span = 1, offset = 0, src, objHeight, objWidth;

    D(bug("[Graph] %s()\n", __func__);)

    rect.MinX = _left(obj);
    rect.MinY = _top(obj);
    rect.MaxX = _right(obj);
    rect.MaxY = _bottom(obj);

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
        if (data->graph_RastPort)
        {
            renderPort = data->graph_RastPort;
            rect.MinX = 0;
            rect.MinY = 0;
            rect.MaxX = _right(obj) - _left(obj);
            rect.MaxY = _bottom(obj) - _top(obj);
        }
        else
            renderPort = _rp(obj);

        objHeight = rect.MaxY - rect.MinY + 1;
        objWidth = rect.MaxX - rect.MinX + 1;

        data->graph_PeriodSize = ((float)objWidth / data->graph_PeriodCeiling) * data->ihn.ihn_Millis;

        if (data->graph_Flags & GRAPHF_PERIODIC)
            offset = data->graph_Tick / data->ihn.ihn_Millis;

        D(
            bug("[Graph] %s: dimensions %d,%d\n", __func__, objWidth, objHeight);
            bug("[Graph] %s: period size %d, span %d\n", __func__, (int)data->graph_PeriodSize, span);
            bug("[Graph] %s: offset %d\n", __func__, offset);
        )

        // First fill the background ..
        SetAPen(renderPort, data->graph_BackPen);
        RectFill(renderPort, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

        if (data->graph_Flags & GRAPHF_DRAWSEGS)
        {
            // Draw the segment divisions..
            SetAPen(renderPort, data->graph_SegmentPen);
            data->graph_SegmentSize = (objWidth * data->graph_PeriodStepping) / data->graph_PeriodCeiling;
            if (data->graph_SegmentSize < 2.0)
                data->graph_SegmentSize = 2.0;
            for (pos = 0; pos <= ((data->graph_PeriodCeiling / data->graph_PeriodStepping) + 1); pos++)
            {
                Move(renderPort, rect.MinX + (pos * data->graph_SegmentSize) - (offset * data->graph_PeriodSize), rect.MinY);
                Draw(renderPort, rect.MinX + (pos * data->graph_SegmentSize) - (offset * data->graph_PeriodSize), rect.MaxY);
            }
            data->graph_SegmentSize = (objHeight * data->graph_ValStepping) / data->graph_ValCeiling;
            if (data->graph_SegmentSize < 2.0)
                data->graph_SegmentSize = 2.0;
            for (pos = rect.MaxY; pos >= rect.MinY; pos -= data->graph_SegmentSize)
            {
                Move(renderPort, rect.MinX, pos);
                Draw(renderPort, rect.MaxX, pos);
            }
        }

        if (data->graph_Flags & GRAPHF_DRAWAXIS)
        {
            // Draw the Axis..
            SetAPen(renderPort, data->graph_AxisPen);
            Move(renderPort, rect.MinX, rect.MinY);
            Draw(renderPort, rect.MaxX, rect.MinY);
            Draw(renderPort, rect.MaxX, rect.MaxY);
            Draw(renderPort, rect.MinX, rect.MaxY);
            Draw(renderPort, rect.MinX, rect.MinY);
        }

        // Plot the entries..
        if (data->graph_Sources) 
        {
            for (src = 0; src < data->graph_SourceCount; src ++)
            {
                UWORD xpos, ypos;
                LONG start;

                sourceData = &data->graph_Sources[src];

                if ((start = data->graph_EntryPtr - (objWidth / data->graph_PeriodSize)) < 0)
                    start = 0;

                ypos = (objHeight * Graph__SumValues(&sourceData->gs_Entries[start], span))/ data->graph_ValCeiling;
                start += span;

                if (data->graph_Flags & GRAPHF_PERIODIC)
                    xpos = rect.MaxX - (data->graph_EntryPtr * data->graph_PeriodSize);
                else
                    xpos = rect.MinX - (offset * data->graph_PeriodSize);

                SetAPen(renderPort, data->graph_Sources[src].gs_PlotPen);

                if (start == 1)
                    WritePixel(renderPort, xpos, rect.MaxY - ypos);

               Move(renderPort, xpos, rect.MaxY - ypos);

                for (pos = start; pos < data->graph_EntryPtr; pos += span)
                {
                    ypos = (objHeight * Graph__SumValues(&sourceData->gs_Entries[pos], span))/ data->graph_ValCeiling;

                    Draw(renderPort,
                        xpos + (pos * data->graph_PeriodSize),
                        rect.MaxY - ypos);
                }
            }
        }

        // Add the InfoText
        pos = ((rect.MinY + rect.MaxY) /2) - ((_font(obj)->tf_YSize * data->graph_ITHeight) /2) + _font(obj)->tf_Baseline;

        if (!IsListEmpty(&data->graph_InfoText))
            SetFont(renderPort, _font(obj));

        ForeachNode(&data->graph_InfoText, infoLine)
        {
            UWORD txtLen = strlen(infoLine->ln_Name);
            UWORD textWidth = TextLength(renderPort, infoLine->ln_Name, txtLen);

            D(bug("[Graph] %s: pos = %d, strlen = %d, wid = %d\n", __func__, pos, txtLen, textWidth);)

            if (textWidth > 0)
            {
                SetAPen(renderPort, _pens(obj)[MPEN_TEXT]);
                Move(renderPort, ((rect.MinX + rect.MaxX) /2) - (textWidth / 2), pos);
                Text(renderPort, (CONST_STRPTR)infoLine->ln_Name, txtLen);
                pos += _font(obj)->tf_YSize;
            }
        }
        if (renderPort != _rp(obj))
        {
            BltBitMapRastPort(
                renderPort->BitMap,
                0,
                0,
                _rp(obj),
                _left(obj),
                _top(obj),
                _right(obj) - _left(obj) + 1,
                _bottom(obj) - _top(obj) + 1,
                0x0C0 );
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
        case MUIV_Graph_Source_PenSrc:
            dataSource->gs_PlotPenSrc = msg->AttribVal;
            break;
        case MUIV_Graph_Source_Pen:
            if (dataSource->gs_PlotPen != -1)
                ReleasePen(_screen(obj)->ViewPort.ColorMap, dataSource->gs_PlotPen);
            dataSource->gs_PlotPen = (WORD)msg->AttribVal;
            break;
        case MUIV_Graph_Source_FillPenSrc:
            dataSource->gs_PlotPen = msg->AttribVal;
            break;
        case MUIV_Graph_Source_FillPen:
            if (dataSource->gs_PlotFillPen != -1)
                ReleasePen(_screen(obj)->ViewPort.ColorMap, dataSource->gs_PlotFillPen);
            dataSource->gs_PlotFillPen = (WORD)msg->AttribVal;
            break;
    }

    return 0;
}

IPTR Graph__MUIM_Graph_Reset(Class *cl, Object *obj, Msg msg)
{
    D(bug("[Graph] %s()\n", __func__);)
    
    return 0;
}

IPTR Graph__MUIM_Graph_Timer(Class *cl, Object *obj, Msg msg)
{
    struct Graph_DATA *data;
    int i;

    D(bug("[Graph] %s()\n", __func__);)

    data = INST_DATA(cl, obj);

    if ((data->graph_Tick += data->ihn.ihn_Millis) > data->graph_PeriodStepping)
        data->graph_Tick -= data->graph_PeriodStepping;

    if (data->graph_Flags & GRAPHF_PERIODIC)
    {
        if (data->graph_SourceCount > 0)
        {
            BOOL updateEntries = FALSE, updated = FALSE, move = FALSE;

            if (data->graph_Flags & GRAPHF_FIXEDLEN) 
            {
                if (data->graph_EntryPtr >= data->graph_EntryCount)
                {
                    data->graph_EntryPtr = data->graph_EntryCount - 1;
                    move = TRUE;
                }
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

                    if (move)
                    {
                        CopyMem(&data->graph_Sources[i].gs_Entries[1], &data->graph_Sources[i].gs_Entries[0], sizeof(IPTR) * (data->graph_EntryCount - 1));
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
