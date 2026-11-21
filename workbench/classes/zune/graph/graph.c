/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.
*/

/****** Graph.mcc/MUIC_Graph ************
*
*   NAME
*       MUIC_Graph
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display time-ordered sampled values from one or more sources as
*       line plots with optional filled areas, grid divisions, and centered
*       information text. Applications supply samples through AROS Hook
*       callbacks. Automatic sampling uses an Application input handler.
*       Manual applications may invoke MUIM_Graph_Timer explicitly.
*
*       The class initially requests one source, but contains no recorded
*       samples. The default value ceiling/step are 100/10; the period
*       ceiling/step are 10000/1000 milliseconds. Automatic sampling is
*       disabled until PeriodInterval is set to a nonzero value. Initial
*       inner margins are four pixels on each side.
*
*   NOTES
*       Source handles returned by MUIM_Graph_GetSourceHandle are pointers
*       to individually allocated records. Adding sources does not move
*       existing records. The handles become invalid when the object is
*       disposed. Keep supplied Hook and RGB data alive as long as used.
*       The class owns sample storage, the copied information-text lines,
*       and screen pens that it explicitly obtains. Numeric pen values
*       provided by the caller are borrowed, not released by the class.
*
*       A nonzero PeriodInterval enables periodic sampling and configures
*       the history capacity from PeriodCeiling/PeriodInterval. Configure
*       positive, representable scales and steps. Setter calls report tag
*       recognition rather than allocation success; unchanged values may
*       also mean an allocation failure. Only the attributes documented as
*       gettable have local OM_GET implementations.
*
*       The graph updates on the Application task; a callback must not
*       dispose the object or restructure its sources while a sample is
*       being taken. Hide releases screen pens; Cleanup removes the timer;
*       Dispose also stops an outstanding handler and releases owned data.
*       Failed area-buffer allocations or histories exceeding the
*       WORD-sized InitArea vector limit omit the filled area; line plots
*       and other graph elements can still be drawn.
*
*   LIMITATIONS
*       Aggregate mode renders the integer mean of all source samples as
*       one series. This class does not implement bar, pie, or scatter
*       diagrams, export, interaction, zooming, or hardware sampling.
*       Source colour acquisition depends on the active screen. Native
*       AROS redraw and resource-lifetime behaviour must be qualified on
*       the actual target; source-level tests are not runtime evidence.
*
*/
/****** Graph.mcc/MUIA_Graph_InfoText ************
*
*   NAME
*       MUIA_Graph_InfoText
*
*   FUNCTION
*       Set multiline text centered over the plotted area.
*
*   INPUTS
*       Initialize, set; pointer to a NUL-terminated string or NULL.
*       Default NULL. No Graph-specific getter.
*
*   NOTES
*       The text is split on newline boundaries and copied into owned
*       per-line storage. NULL clears the text. On allocation failure the
*       existing text remains intact. Lines containing no characters are
*       omitted. No MUI escape interpretation is performed. A shown graph
*       requests an update redraw after the setter.
*
*/
/****** Graph.mcc/MUIA_Graph_EntryCount ************
*
*   NAME
*       MUIA_Graph_EntryCount
*
*   FUNCTION
*       Set or query the number of history slots per source.
*
*   INPUTS
*       Initialize, set, get; IPTR. Initially zero.
*
*   NOTES
*       Positive values resize every source transactionally and enable
*       fixed-length history. Existing recent samples are retained up to
*       the new capacity. A failed allocation preserves the old capacity
*       and arrays. Zero disables fixed-length mode without deleting the
*       existing storage or sample history. In nonfixed mode a sampling
*       tick grows history as required. The getter reports capacity, not
*       the number of valid recorded samples. Reset clears recorded values.
*
*/
/****** Graph.mcc/MUIA_Graph_Aggregate ************
*
*   NAME
*       MUIA_Graph_Aggregate
*
*   FUNCTION
*       Toggle the combined mean-value plot.
*
*   INPUTS
*       Initialize, set; BOOL. Default FALSE. No local getter.
*
*   NOTES
*       FALSE plots each source separately. TRUE plots one series whose
*       sampled values are the integer arithmetic mean of the available
*       source records at each time index. It does not sum measurements,
*       alter source histories, or change the sampling callbacks. The
*       combined series uses source zero's plot and fill pens. Toggling
*       requests a redraw while the graph is shown.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodCeiling ************
*
*   NAME
*       MUIA_Graph_PeriodCeiling
*
*   FUNCTION
*       Set the visible horizontal period in milliseconds.
*
*   INPUTS
*       Initialize, set; positive IPTR. Default 10000 ms.
*       No Graph-specific getter.
*
*   NOTES
*       Zero is ignored. In periodic mode this setter attempts to resize
*       history from the new period and current sample interval before
*       storing the new period. A resize failure preserves the old period.
*       In manual mode it changes only the period. Scale changes request
*       redraw while shown, but do not themselves request MUI relayout.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodStep ************
*
*   NAME
*       MUIA_Graph_PeriodStep
*
*   FUNCTION
*       Set horizontal grid spacing and tick phase in milliseconds.
*
*   INPUTS
*       Initialize, set; positive IPTR. Default 1000 ms.
*       No Graph-specific getter.
*
*   NOTES
*       Zero is ignored. Grid spacing is constrained to a finite number
*       of rendered divisions, even for small requested steps. This tag
*       does not change the sampling interval or sample capacity.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodInterval ************
*
*   NAME
*       MUIA_Graph_PeriodInterval
*
*   FUNCTION
*       Enable, configure, or disable automatic sampling.
*
*   INPUTS
*       Initialize, set, get; IPTR milliseconds. Get returns zero when
*       periodic sampling is disabled. Initially disabled.
*
*   NOTES
*       A positive interval enables periodic sampling, selects fixed
*       capacity based on PeriodCeiling/interval, and registers an input
*       handler after Setup. Changing the interval clears sample history;
*       repeating the existing active interval preserves history and does
*       not re-register the timer. An allocation failure leaves the old
*       live configuration unchanged. Zero removes the timer and disables
*       sampling without deleting stored samples. Timer callbacks execute
*       through the MUI Application event-processing context.
*
*/
/****** Graph.mcc/MUIA_Graph_ValueCeiling ************
*
*   NAME
*       MUIA_Graph_ValueCeiling
*
*   FUNCTION
*       Set the vertical plot ceiling.
*
*   INPUTS
*       Initialize, set, get; positive IPTR. Default 100.
*
*   NOTES
*       Zero is ignored. Values at or above the ceiling are clipped to
*       the upper rendering edge. This changes the representation of
*       existing values, not the recorded sample history.
*
*/
/****** Graph.mcc/MUIA_Graph_ValueStep ************
*
*   NAME
*       MUIA_Graph_ValueStep
*
*   FUNCTION
*       Set vertical grid-division spacing.
*
*   INPUTS
*       Initialize, set; positive IPTR. Default 10.
*       No Graph-specific getter.
*
*   NOTES
*       Zero is ignored. Rendering bounds the number of grid lines and
*       uses the current ValueCeiling to establish relative spacing.
*
*/
/****** Graph.mcc/MUIA_Graph_ManualRefresh ************
*
*   NAME
*       MUIA_Graph_ManualRefresh
*
*   FUNCTION
*       Control automatic redraws following sampling ticks.
*
*   INPUTS
*       Initialize, set; BOOL. Default FALSE. No local getter.
*
*   NOTES
*       FALSE enables an installed PeriodicTick notification that requests
*       MADF_DRAWUPDATE; TRUE suppresses that callback's redraw. Changes
*       take effect on the next notification, including after Setup.
*       This flag does not stop sampling. Applications can request an
*       explicit redraw through the normal MUI interface.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodicTick ************
*
*   NAME
*       MUIA_Graph_PeriodicTick
*
*   FUNCTION
*       Provide a MUI notification trigger for completed sampling ticks.
*
*   INPUTS
*       Set; BOOL trigger value. No Graph-specific getter.
*
*   NOTES
*       MUIM_Graph_Timer sets this attribute after processing sources.
*       Its installed notification may request a redraw unless
*       ManualRefresh is enabled. The attribute is not a history buffer
*       and does not provide a measurement value.
*
*/
/****** Graph.mcc/MUIM_Graph_GetSourceHandle ************
*
*   NAME
*       MUIM_Graph_GetSourceHandle
*
*   FUNCTION
*       Return a borrowed source record handle, creating sources if needed.
*
*   INPUTS
*       SourceNo: ULONG zero-based source index.
*
*   RESULT
*       Nonzero pointer to the source record, or zero on failure.
*
*   NOTES
*       New records and their history are zero-initialized. Source-array
*       growth is transactional; existing returned handles remain valid
*       when other sources are added. A handle remains owned by the graph
*       and is invalid after object disposal. Do not store handles across
*       object lifetimes or pass them to another graph instance.
*
*/
/****** Graph.mcc/MUIM_Graph_SetSourceAttrib ************
*
*   NAME
*       MUIM_Graph_SetSourceAttrib
*
*   FUNCTION
*       Configure a source read hook or its line/fill colour.
*
*   INPUTS
*       SourceHandle: borrowed handle from GetSourceHandle.
*       Attrib: MUIV_Graph_Source_ReadHook, _PenSrc, _Pen,
*           _FillPenSrc, or _FillPen.
*       AttribVal: Hook pointer, RGB triplet pointer, or WORD pen number.
*
*   NOTES
*       Foreign, NULL and stale handles are rejected without dereference.
*       Hook and RGB pointers are borrowed. Numeric pen numbers belong to
*       the caller. RGB-based pens are acquired on the shown screen and
*       released by Graph.mcc when replaced or hidden. The line and fill
*       RGB attributes are independent. A failed RGB reservation does not
*       leave an obsolete borrowed pen active. Requests to change a shown
*       source trigger a redraw. The method returns zero; its return value
*       is not a successful-acquisition or modification indicator.
*
*/
/****** Graph.mcc/MUIM_Graph_Reset ************
*
*   NAME
*       MUIM_Graph_Reset
*
*   FUNCTION
*       Clear recorded measurements without changing source configuration.
*
*   INPUTS
*       No method-specific arguments.
*
*   NOTES
*       Clears every source's sample storage, the valid-sample count and
*       grid-phase tick counter. Configured sources, pens, capacity and
*       timer settings remain in place. A shown graph is redrawn. Return
*       value is zero; callers must not interpret it as a Boolean result.
*
*/
/****** Graph.mcc/MUIM_Graph_Timer ************
*
*   NAME
*       MUIM_Graph_Timer
*
*   FUNCTION
*       Sample every available source and advance the graph's history.
*
*   INPUTS
*       No method-specific arguments. Normally invoked by an Application
*       timer input handler; manual invocation is also possible.
*
*   NOTES
*       Requires an enabled nonzero PeriodInterval and valid source
*       storage. Each hooked source receives a writable IPTR sample slot
*       and its Hook data. Unhooked sources receive zero for that tick.
*       Fixed-length histories shift their oldest values when full;
*       nonfixed histories may grow transactionally. After sampling the
*       method emits PeriodicTick so listeners can observe progress.
*       It does not perform hardware measurements itself. A callback must
*       not reconfigure or dispose its graph while running.
*
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
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>

#include <graphics/gfxmacros.h>
#include <cybergraphx/cybergraphics.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "graph.h"
#include "graph_intern.h"

/* Individual source records never move: a returned handle survives growth. */
IPTR Graph__UpdateSourceArray(struct Graph_DATA *data, IPTR count)
{
    struct Graph_SourceDATA **newSources;
    IPTR i;

    if (count <= data->graph_SourceCount)
        return (IPTR)data->graph_Sources;
    if (count > ((IPTR)-1) / sizeof(*newSources))
        return 0;

    newSources = AllocMem(count * sizeof(*newSources), MEMF_ANY | MEMF_CLEAR);
    if (!newSources)
        return 0;
    if (data->graph_Sources)
        CopyMem(data->graph_Sources, newSources,
                data->graph_SourceCount * sizeof(*newSources));

    for (i = data->graph_SourceCount; i < count; i++)
    {
        newSources[i] = AllocMem(sizeof(**newSources), MEMF_ANY | MEMF_CLEAR);
        if (!newSources[i])
            break;
        newSources[i]->gs_PlotPen = -1;
        newSources[i]->gs_PlotFillPen = -1;
        if (data->graph_EntryCount)
        {
            newSources[i]->gs_Entries = AllocMem(
                data->graph_EntryCount * sizeof(IPTR), MEMF_ANY | MEMF_CLEAR);
            if (!newSources[i]->gs_Entries)
            {
                FreeMem(newSources[i], sizeof(**newSources));
                break;
            }
        }
    }
    if (i != count)
    {
        while (i > data->graph_SourceCount)
        {
            --i;
            if (newSources[i]->gs_Entries)
                FreeMem(newSources[i]->gs_Entries,
                        data->graph_EntryCount * sizeof(IPTR));
            FreeMem(newSources[i], sizeof(**newSources));
        }
        FreeMem(newSources, count * sizeof(*newSources));
        return 0;
    }

    if (data->graph_Sources)
        FreeMem(data->graph_Sources,
                data->graph_SourceCount * sizeof(*newSources));
    data->graph_Sources = newSources;
    data->graph_SourceCount = count;
    return (IPTR)newSources;
}

/* Replace every source's sample storage before changing the shared capacity. */
BOOL Graph__ResizeEntries(struct Graph_DATA *data, IPTR count)
{
    IPTR **arrays;
    IPTR oldValid, copyCount, start, i;

    if (count == data->graph_EntryCount)
        return TRUE;
    if (!count || count > ((IPTR)-1) / sizeof(IPTR) ||
        !data->graph_SourceCount ||
        data->graph_SourceCount > ((IPTR)-1) / sizeof(*arrays))
        return FALSE;

    arrays = AllocMem(data->graph_SourceCount * sizeof(*arrays), MEMF_ANY | MEMF_CLEAR);
    if (!arrays)
        return FALSE;

    oldValid = data->graph_EntryPtr;
    if (oldValid > data->graph_EntryCount)
        oldValid = data->graph_EntryCount;
    copyCount = oldValid < count ? oldValid : count;
    start = oldValid - copyCount;
    for (i = 0; i < data->graph_SourceCount; i++)
    {
        arrays[i] = AllocMem(count * sizeof(IPTR), MEMF_ANY | MEMF_CLEAR);
        if (!arrays[i])
            break;
        if (data->graph_Sources[i]->gs_Entries && copyCount)
            CopyMem(data->graph_Sources[i]->gs_Entries + start,
                    arrays[i], copyCount * sizeof(IPTR));
    }
    if (i != data->graph_SourceCount)
    {
        while (i > 0)
            FreeMem(arrays[--i], count * sizeof(IPTR));
        FreeMem(arrays, data->graph_SourceCount * sizeof(*arrays));
        return FALSE;
    }

    for (i = 0; i < data->graph_SourceCount; i++)
    {
        if (data->graph_Sources[i]->gs_Entries)
            FreeMem(data->graph_Sources[i]->gs_Entries,
                    data->graph_EntryCount * sizeof(IPTR));
        data->graph_Sources[i]->gs_Entries = arrays[i];
    }
    FreeMem(arrays, data->graph_SourceCount * sizeof(*arrays));
    data->graph_EntryCount = count;
    data->graph_EntryPtr = copyCount;
    return TRUE;
}

/* Remove and free every copied text node before replacing it or disposing the object. */
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

/* Build replacement text nodes transactionally; retain prior text if allocation fails. */
IPTR Graph__ParseInfoText(Class *cl, Object *obj, char *infoTxt)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct List parsed;
    struct Node *line, *next;
    IPTR height = 0;
    size_t i, start = 0, length = infoTxt ? strlen(infoTxt) : 0;

    NEWLIST(&parsed);
    if (infoTxt)
    {
        for (i = 0; i <= length; i++)
        {
            if (infoTxt[i] == '\n' || infoTxt[i] == '\0')
            {
                size_t count = i - start;
                if (count)
                {
                    if (count > ((size_t)-1) - sizeof(*line) - 1)
                        goto failed;
                    line = AllocVec(sizeof(*line) + count + 1,
                                    MEMF_ANY | MEMF_CLEAR);
                    if (!line)
                        goto failed;
                    line->ln_Name = (char *)(line + 1);
                    CopyMem(&infoTxt[start], line->ln_Name, count);
                    AddTail(&parsed, line);
                    height++;
                }
                start = i + 1;
            }
        }
    }

    Graph__FreeInfoText(cl, obj);
    while ((line = RemHead(&parsed)) != NULL)
        AddTail(&data->graph_InfoText, line);
    data->graph_ITHeight = height;
    return height;

failed:
    ForeachNodeSafe(&parsed, line, next)
    {
        Remove(line);
        FreeVec(line);
    }
    return data->graph_ITHeight;
}

/* The exact average, without overflowing IPTR while summing sources. */
static IPTR Graph__SampleAt(struct Graph_DATA *data, IPTR sourceNo, IPTR index)
{
    IPTR value = 0, remaining = 0, i, count;

    if (!(data->graph_Flags & GRAPHF_AGGR))
    {
        struct Graph_SourceDATA *source = data->graph_Sources[sourceNo];
        return source->gs_Entries ? source->gs_Entries[index] : 0;
    }
    count = data->graph_SourceCount;
    if (!count)
        return 0;
    for (i = 0; i < count; i++)
    {
        struct Graph_SourceDATA *source = data->graph_Sources[i];
        IPTR sample = source->gs_Entries ? source->gs_Entries[index] : 0;
        IPTR fraction = sample % count;
        value += sample / count;
        if (remaining >= count - fraction)
        {
            value++;
            remaining -= count - fraction;
        }
        else
            remaining += fraction;
    }
    return value;
}

/* Clamp first, then scale: width * raw_sample may overflow on i386. */
static LONG Graph__SampleY(const struct Graph_DATA *data,
                           LONG height, IPTR value)
{
    if (value >= data->graph_ValCeiling)
        return height;
    return (LONG)(((double)value * height) / data->graph_ValCeiling);
}

/* Persist area work buffers across redraws and change them atomically. */
static BOOL Graph__EnsureFillBuffers(struct Graph_DATA *data,
                                    ULONG width, ULONG height, IPTR vectors)
{
    IPTR bytes, vectorBytes;
    APTR newTmp = data->graph_FillTmp, newVectors = data->graph_FillVectors;

    /* AROS InitArea takes a signed WORD MaxVectors (up to 32767). */
    if (!width || !height || !vectors || vectors > 32767)
        return FALSE;
    /* RASSIZE expands width to a 16-pixel boundary before multiplying.
     * Avoid an overflowing macro expression on the 32-bit target. */
    if (width > ((ULONG)-1) - 15 ||
        height > ((ULONG)-1) / (width + 15))
        return FALSE;
    bytes = RASSIZE(width, height);
    vectorBytes = vectors * 5;
    if (!bytes)
        return FALSE;

    if (bytes > data->graph_FillTmpBytes)
    {
        newTmp = AllocMem(bytes, MEMF_CHIP | MEMF_CLEAR);
        if (!newTmp)
            return FALSE;
    }
    if (vectorBytes > data->graph_FillVectorBytes)
    {
        newVectors = AllocVec(vectorBytes, MEMF_ANY | MEMF_CLEAR);
        if (!newVectors)
        {
            if (newTmp != data->graph_FillTmp)
                FreeMem(newTmp, bytes);
            return FALSE;
        }
    }

    if (newTmp != data->graph_FillTmp)
    {
        if (data->graph_FillTmp)
            FreeMem(data->graph_FillTmp, data->graph_FillTmpBytes);
        data->graph_FillTmp = newTmp;
        data->graph_FillTmpBytes = bytes;
    }
    if (newVectors != data->graph_FillVectors)
    {
        if (data->graph_FillVectors)
            FreeVec(data->graph_FillVectors);
        data->graph_FillVectors = newVectors;
        data->graph_FillVectorBytes = vectorBytes;
    }
    return TRUE;
}

static void Graph__ReleasePenReservation(Object *obj, WORD *pen, UBYTE *owned)
{
    if (*owned && *pen >= 0)
        ReleasePen(_screen(obj)->ViewPort.ColorMap, *pen);
    if (*owned)
        *pen = -1;
    *owned = FALSE;
}

static void Graph__AcquireSourcePens(Object *obj, struct Graph_SourceDATA *source)
{
    ULONG *rgb;
    LONG pen;

    if (source->gs_PlotPenSrc && !source->gs_PlotPenOwned)
    {
        rgb = (ULONG *)source->gs_PlotPenSrc;
        pen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                            rgb[0], rgb[1], rgb[2],
                            OBP_Precision, PRECISION_GUI,
                            OBP_FailIfBad, FALSE, TAG_DONE);
        if (pen >= 0)
        {
            source->gs_PlotPen = pen;
            source->gs_PlotPenOwned = TRUE;
        }
    }
    if (source->gs_PlotFillPenSrc && !source->gs_PlotFillPenOwned)
    {
        rgb = (ULONG *)source->gs_PlotFillPenSrc;
        pen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                            rgb[0], rgb[1], rgb[2],
                            OBP_Precision, PRECISION_GUI,
                            OBP_FailIfBad, FALSE, TAG_DONE);
        if (pen >= 0)
        {
            source->gs_PlotFillPen = pen;
            source->gs_PlotFillPenOwned = TRUE;
        }
    }
}

static void Graph__ClearEntries(struct Graph_DATA *data)
{
    IPTR i;
    data->graph_EntryPtr = 0;
    data->graph_Tick = 0;
    for (i = 0; i < data->graph_SourceCount; i++)
        if (data->graph_Sources[i]->gs_Entries && data->graph_EntryCount)
            memset(data->graph_Sources[i]->gs_Entries, 0,
                   data->graph_EntryCount * sizeof(IPTR));
}

static void Graph__StopTimer(Object *obj, struct Graph_DATA *data)
{
    if (data->graph_Flags & GRAPHF_HANDLER)
    {
        DoMethod(_app(obj), MUIM_Application_RemInputHandler,
                 (IPTR)&data->graph_IHN);
        data->graph_Flags &= ~GRAPHF_HANDLER;
    }
}

static void Graph__StartTimer(Object *obj, struct Graph_DATA *data)
{
    if ((data->graph_Flags & (GRAPHF_SETUP | GRAPHF_PERIODIC)) ==
        (GRAPHF_SETUP | GRAPHF_PERIODIC) &&
        !(data->graph_Flags & GRAPHF_HANDLER) &&
        data->graph_IHN.ihn_Millis)
    {
        DoMethod(_app(obj), MUIM_Application_AddInputHandler,
                 (IPTR)&data->graph_IHN);
        data->graph_Flags |= GRAPHF_HANDLER;
    }
}

/* Forward completed periodic ticks to the enabled MUI redraw notification. */
AROS_UFH3(
    void, Graph__HookFunc_DrawUpdateFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(Object *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    D(bug("[Graph] %s(0x%p)\n", __func__, obj));

    if (((struct Graph_DATA *)hook->h_Data)->graph_Flags & GRAPHF_REDRAWUPDATE)
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
/* Initialize a source and default settings, then install a live-configurable redraw notification. */
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

        data->graph_Flags =  (GRAPHF_CLEAR | GRAPHF_REDRAWUPDATE | GRAPHF_DRAWAXIS | GRAPHF_DRAWSEGS);
        data->graph_Flags |= GRAPHF_FILL;

        data->graph_BackPen = -1;
        data->graph_AxisPen = -1;
        data->graph_SegmentPen = -1;

        /* A graph without source storage must not reach drawing or sampling. */
        if (!Graph__UpdateSourceArray(data, 1))
        {
            MUI_DisposeObject(obj);
            return 0;
        }

        data->graph_IHN.ihn_Flags  = MUIIHNF_TIMER;
        data->graph_IHN.ihn_Method = MUIM_Graph_Timer;
        data->graph_IHN.ihn_Object = obj;
        data->graph_IHN.ihn_Millis = 1000;

        data->graph_PeriodCeiling = data->graph_IHN.ihn_Millis * 10;
        data->graph_PeriodStepping = data->graph_IHN.ihn_Millis;

        data->graph_ValCeiling = 100;
        data->graph_ValStepping = 10;

        SetAttrsA(obj, msg->ops_AttrList);

        /* Keep the notification installed. The callback consults the live
         * ManualRefresh setting instead of relying on constructor state. */
        data->graph_DrawUpdateHook.h_Entry = (HOOKFUNC)Graph__HookFunc_DrawUpdateFunc;
        data->graph_DrawUpdateHook.h_Data = data;
        DoMethod(obj, MUIM_Notify, MUIA_Graph_PeriodicTick, TRUE,
                 (IPTR)obj, 3,
                 MUIM_CallHook, &data->graph_DrawUpdateHook, (IPTR)NULL);
    }

    return (IPTR)obj;
}

/* Stop a remaining input handler, release owned source memory, pens and drawing buffers. */
IPTR Graph__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR i;

    Graph__StopTimer(obj, data);
    if (data->graph_Flags & GRAPHF_SHOW)
    {
        if (data->graph_BackPen >= 0)
            ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_BackPen);
        if (data->graph_AxisPen >= 0)
            ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_AxisPen);
        if (data->graph_SegmentPen >= 0)
            ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_SegmentPen);
    }
    if (data->graph_SourceCount)
    {
        for (i = 0; i < data->graph_SourceCount; i++)
        {
            struct Graph_SourceDATA *source = data->graph_Sources[i];
            if (data->graph_Flags & GRAPHF_SHOW)
            {
                Graph__ReleasePenReservation(obj, &source->gs_PlotPen,
                                             &source->gs_PlotPenOwned);
                Graph__ReleasePenReservation(obj, &source->gs_PlotFillPen,
                                             &source->gs_PlotFillPenOwned);
            }
            if (source->gs_Entries)
                FreeMem(source->gs_Entries,
                        data->graph_EntryCount * sizeof(IPTR));
            FreeMem(source, sizeof(*source));
        }
        FreeMem(data->graph_Sources,
                data->graph_SourceCount * sizeof(*data->graph_Sources));
    }
    if (data->graph_FillTmp)
        FreeMem(data->graph_FillTmp, data->graph_FillTmpBytes);
    if (data->graph_FillVectors)
        FreeVec(data->graph_FillVectors);

    Graph__FreeInfoText(cl, obj);
    return DoSuperMethodA(cl, obj, msg);
}


/* Apply Graph tags in order; recognize each tag without promising allocation-success status. */
IPTR Graph__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags  = msg->ops_AttrList;
    struct TagItem       *tag;
    IPTR                  count = 0;
    BOOL                  redraw = FALSE;

    D(bug("[Graph] %s()\n", __func__);)

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case MUIA_Graph_PeriodicTick:
                count++;
                break;

            /* Aggregate mode plots the mean of the available source values. */
            case MUIA_Graph_Aggregate:
                data->graph_Flags &= ~GRAPHF_AGGR;
                if (tag->ti_Data)
                    data->graph_Flags |= GRAPHF_AGGR;
                redraw = TRUE;
                count++;
                break;

            /* Set the info text to display */
            case MUIA_Graph_InfoText:
                Graph__ParseInfoText(cl, obj, (char *)tag->ti_Data);
                redraw = TRUE;
                count++;
                break;

            /* Set the input value ceiling and stepping */
            case MUIA_Graph_ValueCeiling:
                if (tag->ti_Data)
                {
                    data->graph_ValCeiling = tag->ti_Data;
                    redraw = TRUE;
                }
                count++;
                break;

            case MUIA_Graph_ValueStep:
                if (tag->ti_Data)
                {
                    data->graph_ValStepping = tag->ti_Data;
                    redraw = TRUE;
                }
                count++;
                break;

            /* Set the period ceiling and stepping */
            case MUIA_Graph_PeriodCeiling:
                if (tag->ti_Data)
                {
                    IPTR entries = data->graph_IHN.ihn_Millis ?
                        tag->ti_Data / data->graph_IHN.ihn_Millis : 0;
                    if (!entries)
                        entries = 1;
                    if (!(data->graph_Flags & GRAPHF_PERIODIC) ||
                        Graph__ResizeEntries(data, entries))
                    {
                        data->graph_PeriodCeiling = tag->ti_Data;
                        redraw = TRUE;
                    }
                }
                count++;
                break;

            case MUIA_Graph_PeriodStep:
                if (tag->ti_Data)
                {
                    data->graph_PeriodStepping = tag->ti_Data;
                    redraw = TRUE;
                }
                count++;
                break;

            case MUIA_Graph_PeriodInterval:
                if (tag->ti_Data &&
                    (data->graph_Flags & GRAPHF_PERIODIC) &&
                    data->graph_IHN.ihn_Millis == tag->ti_Data)
                {
                    /* No change: avoid unhooking/reinstalling a live timer. */
                }
                else if (tag->ti_Data)
                {
                    IPTR entries = data->graph_PeriodCeiling / tag->ti_Data;
                    if (!entries)
                        entries = 1;
                    if (Graph__ResizeEntries(data, entries))
                    {
                        /* Stop before changing the live handler node. A changed
                         * sample interval invalidates old timestamps. */
                        Graph__StopTimer(obj, data);
                        if (data->graph_IHN.ihn_Millis != tag->ti_Data)
                            Graph__ClearEntries(data);
                        data->graph_IHN.ihn_Millis = tag->ti_Data;
                        data->graph_Flags |= GRAPHF_PERIODIC | GRAPHF_FIXEDLEN;
                        Graph__StartTimer(obj, data);
                        redraw = TRUE;
                    }
                }
                else
                {
                    Graph__StopTimer(obj, data);
                    data->graph_Flags &= ~GRAPHF_PERIODIC;
                    data->graph_IHN.ihn_Millis = 0;
                    data->graph_Tick = 0;
                    redraw = TRUE;
                }
                count++;
                break;

            /* Set or turn off Fixed entry count mode */
            case MUIA_Graph_EntryCount:
                if (tag->ti_Data)
                {
                    if (Graph__ResizeEntries(data, tag->ti_Data))
                    {
                        data->graph_Flags |= GRAPHF_FIXEDLEN;
                        redraw = TRUE;
                    }
                }
                else
                {
                    data->graph_Flags &= ~GRAPHF_FIXEDLEN;
                    redraw = TRUE;
                }
                count++;
                break;

            case MUIA_Graph_ManualRefresh:
                if (tag->ti_Data)
                {
                    data->graph_Flags &= ~GRAPHF_REDRAWUPDATE;
                }
                else
                {
                    data->graph_Flags |= GRAPHF_REDRAWUPDATE;
                }
                count++;
                break;
        }
    }

    if ((data->graph_Flags & GRAPHF_SHOW) && (redraw))
        MUI_Redraw(obj, MADF_DRAWUPDATE);

    return count + DoSuperMethodA(cl, obj, (Msg)msg);
}


/* Handle only ValueCeiling, EntryCount and PeriodInterval locally; delegate other attributes. */
IPTR Graph__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR              retval = TRUE;

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
            *(msg->opg_Storage) = (data->graph_Flags & GRAPHF_PERIODIC) ?
                                   data->graph_IHN.ihn_Millis : 0;
            break;

        default:
            retval = DoSuperMethodA(cl, obj, (Msg)msg);
            break;
    }
    
    return retval;
}

/* Complete superclass Setup before registering an enabled periodic input handler. */
IPTR Graph__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    D(bug("[Graph] %s()\n", __func__);)

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    /* Register the handler only after the superclass has completed setup. */
    data->graph_Flags |= GRAPHF_SETUP;
    Graph__StartTimer(obj, data);

    return TRUE;
}


/* Acquire background and source colour reservations for the active screen. */
IPTR Graph__MUIM_Show(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR i;

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return FALSE;

    data->graph_BackPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                              0xF2F2F2F2, 0xF8F8F8F8, 0xFAFAFAFA,
                              OBP_Precision, PRECISION_GUI,
                              OBP_FailIfBad, FALSE, TAG_DONE);
    data->graph_AxisPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                              0x7A7A7A7A, 0xC5C5C5C5, 0xDEDEDEDE,
                              OBP_Precision, PRECISION_GUI,
                              OBP_FailIfBad, FALSE, TAG_DONE);
    data->graph_SegmentPen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap,
                              0x85858585, 0xD3D3D3D3, 0xEDEDEDED,
                              OBP_Precision, PRECISION_GUI,
                              OBP_FailIfBad, FALSE, TAG_DONE);
    for (i = 0; i < data->graph_SourceCount; i++)
        Graph__AcquireSourcePens(obj, data->graph_Sources[i]);

    data->graph_Flags |= GRAPHF_SHOW;
    return TRUE;
}

/* Release owned screen pens and preserve caller-owned numeric pen values. */
IPTR Graph__MUIM_Hide(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR i;

    data->graph_Flags &= ~GRAPHF_SHOW;
    if (data->graph_SegmentPen >= 0)
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_SegmentPen);
    if (data->graph_AxisPen >= 0)
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_AxisPen);
    if (data->graph_BackPen >= 0)
        ReleasePen(_screen(obj)->ViewPort.ColorMap, data->graph_BackPen);
    data->graph_SegmentPen = -1;
    data->graph_AxisPen = -1;
    data->graph_BackPen = -1;

    for (i = 0; i < data->graph_SourceCount; i++)
    {
        struct Graph_SourceDATA *source = data->graph_Sources[i];
        Graph__ReleasePenReservation(obj, &source->gs_PlotPen,
                                     &source->gs_PlotPenOwned);
        Graph__ReleasePenReservation(obj, &source->gs_PlotFillPen,
                                     &source->gs_PlotFillPenOwned);
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Stop the input handler before delegating to superclass Cleanup. */
IPTR Graph__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    Graph__StopTimer(obj, data);
    data->graph_Flags &= ~GRAPHF_SETUP;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/* Compute bounded preferred dimensions from the configured scale ratios. */
IPTR Graph__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR x, y;

    DoSuperMethodA(cl, obj, (Msg)msg);
    x = data->graph_PeriodStepping ?
        data->graph_PeriodCeiling / data->graph_PeriodStepping : 1;
    y = data->graph_ValStepping ?
        data->graph_ValCeiling / data->graph_ValStepping : 1;
    if (x > 64) x = 64;
    if (y > 64) y = 64;
    if (x < 1) x = 1;
    if (y < 1) y = 1;
    msg->MinMaxInfo->MinWidth  += x * 2;
    msg->MinMaxInfo->MinHeight += y * 2;
    msg->MinMaxInfo->DefWidth  += x * 10;
    msg->MinMaxInfo->DefHeight += y * 10;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;
    return TRUE;
}

/* Draw bounded history, area fill, grid, frame and copied information text. */
IPTR Graph__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct RastPort *renderPort;
    struct Region *region;
    struct Node *line;
    struct Rectangle rect;
    APTR clip = NULL;
    IPTR sourceNo, sourceLimit, start, i, valid;
    LONG width, height, y, x, pos;
    double pixelsPerSample, gridPixels, phase;
    BOOL hasData;

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
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        goto finished;

    renderPort = data->graph_RastPort ? data->graph_RastPort : _rp(obj);
    if (data->graph_RastPort)
    {
        rect.MinX = rect.MinY = 0;
        rect.MaxX = _right(obj) - _left(obj);
        rect.MaxY = _bottom(obj) - _top(obj);
    }
    width = (LONG)rect.MaxX - rect.MinX + 1;
    height = (LONG)rect.MaxY - rect.MinY + 1;
    if (width <= 0 || height <= 0 || !data->graph_PeriodCeiling ||
        !data->graph_ValCeiling)
        goto finished;

    pixelsPerSample = ((double)width *
        (data->graph_IHN.ihn_Millis ? data->graph_IHN.ihn_Millis :
                                     data->graph_PeriodStepping)) /
        data->graph_PeriodCeiling;
    data->graph_PeriodSize = pixelsPerSample;
    valid = data->graph_EntryPtr;
    if (valid > data->graph_EntryCount)
        valid = data->graph_EntryCount;
    hasData = data->graph_Sources && valid && pixelsPerSample > 0.0;
    start = 0;
    if (hasData && ((double)valid * pixelsPerSample > width + pixelsPerSample))
        start = valid - (IPTR)((double)width / pixelsPerSample) - 2;
    if (start > valid)
        start = 0;
    sourceLimit = (data->graph_Flags & GRAPHF_AGGR) ? 1 : data->graph_SourceCount;

    if (data->graph_Flags & GRAPHF_CLEAR)
    {
        SetAPen(renderPort, data->graph_BackPen >= 0 ? data->graph_BackPen :
                            _pens(obj)[MPEN_BACKGROUND]);
        RectFill(renderPort, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
    }

    if (hasData && valid - start >= 2 &&
        (data->graph_Flags & GRAPHF_FILL) &&
        valid - start <= ((IPTR)-1) - 4 &&
        Graph__EnsureFillBuffers(data, width, height, valid - start + 4))
    {
        struct RastPort *areaPort = CloneRastPort(renderPort);
        if (areaPort)
        {
            struct AreaInfo info;
            struct TmpRas tmp;
            UWORD dither[] = { 0x5555, 0xAAAA };

            memset(&info, 0, sizeof(info));
            memset(data->graph_FillTmp, 0, RASSIZE(width, height));
            memset(data->graph_FillVectors, 0, (valid - start + 4) * 5);
            InitArea(&info, data->graph_FillVectors, valid - start + 4);
            InitTmpRas(&tmp, data->graph_FillTmp, data->graph_FillTmpBytes);
            areaPort->AreaInfo = &info;
            areaPort->TmpRas = &tmp;

            for (sourceNo = 0; sourceNo < sourceLimit; sourceNo++)
            {
                struct Graph_SourceDATA *source = data->graph_Sources[sourceNo];
                BOOL first = TRUE;
                LONG firstX = 0, lastX = 0;
                if (!source->gs_Entries)
                    continue;
                if (source->gs_PlotFillPen >= 0)
                {
                    SetAPen(areaPort, source->gs_PlotFillPen);
                    SetAfPt(areaPort, NULL, 0);
                }
                else
                {
                    SetAPen(areaPort, source->gs_PlotPen >= 0 ?
                            source->gs_PlotPen : _pens(obj)[MPEN_TEXT]);
                    SetAfPt(areaPort, dither, 1);
                }
                for (i = start; i < valid; i++)
                {
                    {
                        double sampleX = rect.MaxX -
                            (double)(valid - i) * pixelsPerSample;
                        x = sampleX <= rect.MinX ? rect.MinX :
                            (sampleX >= rect.MaxX ? rect.MaxX : (LONG)sampleX);
                    }
                    y = rect.MaxY - Graph__SampleY(data, height - 1,
                                                   Graph__SampleAt(data, sourceNo, i));
                    if (first)
                    {
                        firstX = x;
                        AreaMove(areaPort, x, rect.MaxY);
                        AreaDraw(areaPort, x, y);
                        first = FALSE;
                    }
                    else
                        AreaDraw(areaPort, x, y);
                    lastX = x;
                }
                if (!first)
                {
                    AreaDraw(areaPort, lastX, rect.MaxY);
                    AreaDraw(areaPort, firstX, rect.MaxY);
                    AreaEnd(areaPort);
                }
            }
            areaPort->AreaInfo = NULL;
            areaPort->TmpRas = NULL;
            FreeRastPort(areaPort);
        }
    }

    if (data->graph_Flags & GRAPHF_DRAWSEGS)
    {
        SetAPen(renderPort, data->graph_SegmentPen >= 0 ?
                           data->graph_SegmentPen : _pens(obj)[MPEN_SHADOW]);
        gridPixels = (double)width * data->graph_PeriodStepping /
                     data->graph_PeriodCeiling;
        if (gridPixels < 2.0) gridPixels = 2.0;
        if (gridPixels > width) gridPixels = width;
        /* Normalize the scrolling offset to one rendered grid step.
         * Handles steps larger than the visible period without losing lines. */
        phase = ((data->graph_Flags & GRAPHF_PERIODIC) &&
                 data->graph_PeriodStepping) ?
            (gridPixels * (double)(data->graph_Tick %
                                   data->graph_PeriodStepping) /
             data->graph_PeriodStepping) : 0.0;
        for (pos = 0; pos <= width / gridPixels + 1; pos++)
        {
            x = rect.MinX + (LONG)(pos * gridPixels - phase);
            if (x >= rect.MinX && x <= rect.MaxX)
            {
                Move(renderPort, x, rect.MinY);
                Draw(renderPort, x, rect.MaxY);
            }
        }
        gridPixels = (double)height * data->graph_ValStepping /
                     data->graph_ValCeiling;
        if (gridPixels < 2.0) gridPixels = 2.0;
        if (gridPixels > height) gridPixels = height;
        for (pos = 0; pos <= height / gridPixels; pos++)
        {
            y = rect.MaxY - (LONG)(pos * gridPixels);
            Move(renderPort, rect.MinX, y);
            Draw(renderPort, rect.MaxX, y);
        }
    }

    if (data->graph_Flags & GRAPHF_DRAWAXIS)
    {
        SetAPen(renderPort, data->graph_AxisPen >= 0 ?
                           data->graph_AxisPen : _pens(obj)[MPEN_TEXT]);
        Move(renderPort, rect.MinX, rect.MinY);
        Draw(renderPort, rect.MaxX, rect.MinY);
        Draw(renderPort, rect.MaxX, rect.MaxY);
        Draw(renderPort, rect.MinX, rect.MaxY);
        Draw(renderPort, rect.MinX, rect.MinY);
    }

    if (hasData)
    {
        for (sourceNo = 0; sourceNo < sourceLimit; sourceNo++)
        {
            struct Graph_SourceDATA *source = data->graph_Sources[sourceNo];
            BOOL first = TRUE;
            if (!source->gs_Entries)
                continue;
            SetAPen(renderPort, source->gs_PlotPen >= 0 ?
                               source->gs_PlotPen : _pens(obj)[MPEN_TEXT]);
            for (i = start; i < valid; i++)
            {
                {
                    double sampleX = rect.MaxX -
                        (double)(valid - i) * pixelsPerSample;
                    x = sampleX <= rect.MinX ? rect.MinX :
                        (sampleX >= rect.MaxX ? rect.MaxX : (LONG)sampleX);
                }
                y = rect.MaxY - Graph__SampleY(data, height - 1,
                                               Graph__SampleAt(data, sourceNo, i));
                if (first)
                {
                    WritePixel(renderPort, x, y);
                    Move(renderPort, x, y);
                    first = FALSE;
                }
                else
                    Draw(renderPort, x, y);
            }
        }
    }

    if (!IsListEmpty(&data->graph_InfoText))
        SetFont(renderPort, _font(obj));
    SetAPen(renderPort, _pens(obj)[MPEN_TEXT]);
    pos = rect.MinY + (height - _font(obj)->tf_YSize * data->graph_ITHeight)/2
          + _font(obj)->tf_Baseline;
    ForeachNode(&data->graph_InfoText, line)
    {
        LONG length = strlen(line->ln_Name);
        LONG textWidth = TextLength(renderPort, line->ln_Name, length);
        if (textWidth > 0)
        {
            Move(renderPort, rect.MinX + (width - textWidth)/2, pos);
            Text(renderPort, (CONST_STRPTR)line->ln_Name, length);
            pos += _font(obj)->tf_YSize;
        }
    }

    if (renderPort != _rp(obj))
    {
        BltBitMapRastPort(renderPort->BitMap, 0, 0, _rp(obj),
                          _left(obj), _top(obj), width, height, 0x0C0);
    }

finished:
    if (region)
        MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
    return 0;
}

/* Grow records transactionally and return a stable, borrowed source pointer. */
IPTR Graph__MUIM_Graph_GetSourceHandle(Class *cl, Object *obj,
                                     struct MUIP_Graph_GetSourceHandle *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    if (msg->SourceNo == (ULONG)-1)
        return 0;
    if (msg->SourceNo >= data->graph_SourceCount &&
        !Graph__UpdateSourceArray(data, (IPTR)msg->SourceNo + 1))
        return 0;

    return (IPTR)data->graph_Sources[msg->SourceNo];
}

/* Validate borrowed handle identity before changing Hook or pen configuration. */
IPTR Graph__MUIM_Graph_SetSourceAttrib(Class *cl, Object *obj,
                                     struct MUIP_Graph_SetSourceAttrib *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    struct Graph_SourceDATA *source = (struct Graph_SourceDATA *)msg->SourceHandle;
    IPTR i;

    /* Reject stale, foreign or null borrowed handles without dereferencing. */
    for (i = 0; i < data->graph_SourceCount; i++)
        if (source && data->graph_Sources[i] == source)
            break;
    if (i == data->graph_SourceCount)
        return 0;

    switch (msg->Attrib)
    {
        case MUIV_Graph_Source_ReadHook:
            source->gs_ReadHook = (struct Hook *)msg->AttribVal;
            break;
        case MUIV_Graph_Source_PenSrc:
            if (data->graph_Flags & GRAPHF_SHOW)
                Graph__ReleasePenReservation(obj, &source->gs_PlotPen,
                                             &source->gs_PlotPenOwned);
            /* The former borrowed pen must not survive a mode switch, even
             * when RGB pen acquisition later fails. */
            source->gs_PlotPen = -1;
            source->gs_PlotPenSrc = msg->AttribVal;
            if ((data->graph_Flags & GRAPHF_SHOW) && msg->AttribVal)
                Graph__AcquireSourcePens(obj, source);
            break;
        case MUIV_Graph_Source_Pen:
            if (data->graph_Flags & GRAPHF_SHOW)
                Graph__ReleasePenReservation(obj, &source->gs_PlotPen,
                                             &source->gs_PlotPenOwned);
            source->gs_PlotPenSrc = 0;
            source->gs_PlotPen = (WORD)msg->AttribVal;
            break;
        case MUIV_Graph_Source_FillPenSrc:
            if (data->graph_Flags & GRAPHF_SHOW)
                Graph__ReleasePenReservation(obj, &source->gs_PlotFillPen,
                                             &source->gs_PlotFillPenOwned);
            source->gs_PlotFillPen = -1;
            source->gs_PlotFillPenSrc = msg->AttribVal;
            if ((data->graph_Flags & GRAPHF_SHOW) && msg->AttribVal)
                Graph__AcquireSourcePens(obj, source);
            break;
        case MUIV_Graph_Source_FillPen:
            if (data->graph_Flags & GRAPHF_SHOW)
                Graph__ReleasePenReservation(obj, &source->gs_PlotFillPen,
                                             &source->gs_PlotFillPenOwned);
            source->gs_PlotFillPenSrc = 0;
            source->gs_PlotFillPen = (WORD)msg->AttribVal;
            break;
    }
    if (data->graph_Flags & GRAPHF_SHOW)
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    return 0;
}

/* Clear all recorded values and tick phase without disposing source configuration. */
IPTR Graph__MUIM_Graph_Reset(Class *cl, Object *obj, Msg msg)
{
    D(bug("[Graph] %s()\n", __func__);)

    struct Graph_DATA *data = INST_DATA(cl, obj);

    Graph__ClearEntries(data);
    if (data->graph_Flags & GRAPHF_SHOW)
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    return 0;
}

/* Read active source hooks into bounded history and dispatch PeriodicTick. */
IPTR Graph__MUIM_Graph_Timer(Class *cl, Object *obj, Msg msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    IPTR i;
    BOOL move = FALSE;

    if (!(data->graph_Flags & GRAPHF_PERIODIC) ||
        !data->graph_IHN.ihn_Millis || !data->graph_SourceCount ||
        !data->graph_Sources || !data->graph_PeriodStepping)
        return 0;

    if (data->graph_IHN.ihn_Millis >= data->graph_PeriodStepping ||
        data->graph_Tick >= data->graph_PeriodStepping - data->graph_IHN.ihn_Millis)
        data->graph_Tick = 0;
    else
        data->graph_Tick += data->graph_IHN.ihn_Millis;

    if (data->graph_Flags & GRAPHF_FIXEDLEN)
    {
        if (!data->graph_EntryCount)
            return 0;
        if (data->graph_EntryPtr >= data->graph_EntryCount)
        {
            data->graph_EntryPtr = data->graph_EntryCount - 1;
            move = TRUE;
        }
    }
    else if (data->graph_EntryPtr >= data->graph_EntryCount)
    {
        IPTR next = data->graph_EntryCount ? data->graph_EntryCount : 16;
        if (next == data->graph_EntryCount)
        {
            if (next > ((IPTR)-1) / 2)
                return 0;
            next *= 2;
        }
        if (!Graph__ResizeEntries(data, next))
            return 0;
    }

    for (i = 0; i < data->graph_SourceCount; i++)
    {
        struct Graph_SourceDATA *source = data->graph_Sources[i];
        if (!source->gs_Entries)
            continue;
        if (move && data->graph_EntryCount > 1)
            memmove(source->gs_Entries, source->gs_Entries + 1,
                    (data->graph_EntryCount - 1) * sizeof(IPTR));
        source->gs_Entries[data->graph_EntryPtr] = 0;
        if (source->gs_ReadHook)
            CALLHOOKPKT(source->gs_ReadHook,
                        (APTR)&source->gs_Entries[data->graph_EntryPtr],
                        source->gs_ReadHook->h_Data);
    }
    data->graph_EntryPtr++;
    SET(obj, MUIA_Graph_PeriodicTick, TRUE);
    return 0;
}
