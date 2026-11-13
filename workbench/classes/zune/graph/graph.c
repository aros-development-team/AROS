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
*       Display one or more sampled value series with filled plots, grid
*       divisions, and optional centered information text. Sampling uses
*       caller-supplied hooks and an Application timer handler. The source
*       configuration declares version 0.9.
*       Defaults are ValueCeiling 100, ValueStep 10, PeriodCeiling 10000
*       milliseconds, PeriodStep 1000 milliseconds, and a stored PeriodInterval
*       of 1000 milliseconds. Periodic sampling is initially disabled. One
*       source record is requested at construction, but no sample capacity is
*       allocated by default. Configure valid sample storage and sources before
*       displaying or sampling; a default object is not a ready-to-draw graph.
*       The constructor supplies four-pixel inner margins, initializes its
*       state, then applies caller attributes through OM_SET. Tag order matters.
*       Setup registers an enabled timer; Cleanup removes it. Hide releases
*       display pens but does not stop sampling. Timer delivery follows
*       Application event processing, not elapsed wall-clock measurement.
*       The class owns source records, sample arrays, and copied
*       information-text lines. Hooks and RGB source arrays are borrowed.
*       Complete Hide/Cleanup before disposal; disposal frees data but does not
*       independently remove the timer or release shown-screen pens.
*
*   NOTES
*       Use positive, representable period/value scales and steps. Keep sample
*       counts, source counts, values, and display dimensions within the integer
*       ranges used by drawing. Values are not clamped to ValueCeiling.
*       Minimum/default sizes are derived from ceiling/step ratios; scale
*       changes do not themselves request relayout.
*       For multiple sources, create consecutive indices before obtaining
*       handles for configuration. Adding a source reallocates the record array,
*       invalidating earlier handles. Configure stable sources and capacity
*       before use. Callback code must not dispose the graph or change
*       source/storage structure while sampling is in progress.
*       OM_SET returns the number of recognized tags plus the superclass result,
*       not a reliable success indicator. Source creation, sample allocation,
*       information-text allocation, pen acquisition, and drawing failures do
*       not share a checked error-reporting contract.
*
*   BUGS
*       Aggregate stores a flag that neither sampling nor drawing uses. Reset is
*       a no-op. ManualRefresh does not dynamically add or remove the redraw
*       notification installed during construction.
*       Source-array growth initializes only the final new record; skipping
*       source indices leaves intermediate records uninitialized. Allocation
*       failure is not checked before GetSourceHandle returns an address.
*       Entry-array growth and logical capacity updates are not transactional;
*       shrinking capacity changes the recorded size without resizing
*       allocations.
*       Drawing assumes usable source arrays and scale divisors and can access
*       samples even before a successful read. Unhooked sources are not
*       automatically populated. The information-text parser dereferences
*       allocation results without checking them.
*       Pen handling has lifecycle limits: source pen numbers are not reset on
*       Hide, and equal line/fill pen numbers share a release path regardless of
*       how reservations were acquired. SetSourceAttrib's FillPenSrc case writes
*       the line-pen field instead of the fill RGB pointer. These are existing
*       implementation limits, not supported alternate behaviors.
*
*/
/****** Graph.mcc/MUIA_Graph_InfoText ************
*
*   NAME
*       MUIA_Graph_InfoText
*
*   FUNCTION
*       Replace the centered text drawn over the graph.
*
*   INPUTS
*       Initialize, set; CONST_STRPTR to a readable NUL-terminated string, or
*       NULL. Default NULL. No local getter.
*
*   NOTES
*       The parser first frees all old text nodes, then copies text into
*       per-line allocations. NULL clears the text; source storage is not
*       retained. Empty strings and consecutive or trailing newlines do not have
*       a consistent blank-line layout contract. This uses graphics Text, not
*       MUI text escape processing.
*       Setting while shown requests an update redraw. Allocation failure is
*       unchecked and previous text is not restored. Use bounded
*       application-owned text. The original pointer is forwarded in the
*       attribute tag; a notification recipient must not assume it points to
*       retained line storage.
*
*/
/****** Graph.mcc/MUIA_Graph_EntryCount ************
*
*   NAME
*       MUIA_Graph_EntryCount
*
*   FUNCTION
*       Select fixed history capacity or disable fixed-length mode.
*
*   INPUTS
*       Initialize, set, get; IPTR. Initial stored count zero. A positive value
*       enables fixed-length mode; zero disables that mode.
*
*   NOTES
*       A positive value grows each existing source array only when larger than
*       the stored count, then records the requested count regardless of
*       allocation success. Full fixed histories shift older samples before
*       writing the newest slot. Get reports the stored count, not the number of
*       populated samples.
*       Zero clears the fixed-length flag without clearing the count, arrays, or
*       write position. In nonfixed mode, Timer grows arrays for hooked sources
*       as needed. There is no public setter for individual samples.
*       Changing capacity does not reset history. Smaller positive counts do not
*       resize arrays, yet later copying and freeing use the changed logical
*       count. Growth failures can leave capacity and allocations inconsistent.
*       Set a suitable capacity before use; this is not a safe arbitrary resize
*       or clear operation.
*
*/
/****** Graph.mcc/MUIA_Graph_Aggregate ************
*
*   NAME
*       MUIA_Graph_Aggregate
*
*   FUNCTION
*       Store the aggregate-mode flag.
*
*   INPUTS
*       Initialize, set; BOOL, default FALSE. No local getter.
*
*   NOTES
*       Nonzero enables GRAPHF_AGGR and zero clears it. No code in the sampling
*       or drawing paths consumes this flag. It does not sum or average the
*       sources and does not alter displayed series. The setter comment
*       describes an intention, not implemented aggregation.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodCeiling ************
*
*   NAME
*       MUIA_Graph_PeriodCeiling
*
*   FUNCTION
*       Set the horizontal time scale.
*
*   INPUTS
*       Initialize, set; IPTR milliseconds, default 10000. No local getter.
*
*   NOTES
*       Drawing scales sample spacing using PeriodInterval/PeriodCeiling;
*       measurement uses PeriodCeiling/PeriodStep. Supply a positive value with
*       representable calculations. No validation, redraw, or relayout is
*       requested by this setter alone.
*       Changing this value does not recalculate EntryCount. When enabling
*       PeriodInterval, the setter uses the ceiling already stored at that point
*       to request ceiling/interval entries. Put PeriodCeiling before
*       PeriodInterval when configuring both in one tag list.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodStep ************
*
*   NAME
*       MUIA_Graph_PeriodStep
*
*   FUNCTION
*       Set the horizontal grid and tick-step value.
*
*   INPUTS
*       Initialize, set; IPTR milliseconds, default 1000. No local getter.
*
*   NOTES
*       Measurement divides PeriodCeiling by this value; drawing uses it for
*       vertical divisions. Timer adds the stored interval to its tick counter
*       and subtracts PeriodStep once only when the result is greater than
*       PeriodStep. This is not a general modulo reduction or a wall-clock
*       timestamp.
*       Use a positive value and suitable scale ratios. The setter does not
*       validate divisors or request redraw or relayout. Changing PeriodStep
*       does not change the Application timer interval or sample capacity.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodInterval ************
*
*   NAME
*       MUIA_Graph_PeriodInterval
*
*   FUNCTION
*       Enable periodic sampling with the specified interval, or disable it.
*
*   INPUTS
*       Initialize, set, get; IPTR milliseconds. Initial stored interval 1000,
*       but periodic mode initially disabled. Zero disables periodic mode.
*
*   NOTES
*       A nonzero value enables periodic mode, assigns the timer-node interval,
*       and sets EntryCount to the integer quotient PeriodCeiling/interval. If
*       already set up and not registered, it adds the Application handler.
*       Configure a ceiling and interval that yield a positive, usable capacity.
*       Zero removes an active handler while set up and clears periodic mode,
*       but preserves the stored interval. Get therefore returns the previous
*       interval after disabling; it does not report whether sampling is
*       enabled.
*       Changing a nonzero interval while already registered edits the node
*       without removing and re-adding it. No rescheduling guarantee is
*       provided. EntryCount updates are nested attribute sets and can notify
*       separately. Disabling does not clear samples or reset the write
*       position.
*
*/
/****** Graph.mcc/MUIA_Graph_ValueCeiling ************
*
*   NAME
*       MUIA_Graph_ValueCeiling
*
*   FUNCTION
*       Set the value represented by the full graph height.
*
*   INPUTS
*       Initialize, set, get; IPTR, default 100.
*
*   NOTES
*       Drawing scales samples by object height divided by this ceiling;
*       measurement divides it by ValueStep. Supply a positive ceiling and
*       values appropriate to that scale. No clamping, auto-ranging, or
*       validation occurs.
*       The getter returns the stored ceiling. Setting it alone requests neither
*       redraw nor relayout; the next draw uses the new scale. Large
*       intermediate products and coordinates are not protected against
*       overflow.
*
*/
/****** Graph.mcc/MUIA_Graph_ValueStep ************
*
*   NAME
*       MUIA_Graph_ValueStep
*
*   FUNCTION
*       Set the spacing of horizontal value divisions.
*
*   INPUTS
*       Initialize, set; IPTR, default 10. No local getter.
*
*   NOTES
*       Measurement derives minimum/default height from ValueCeiling/ValueStep.
*       Drawing derives grid spacing from object height times ValueStep divided
*       by ValueCeiling, with a two-pixel lower bound. Supply a positive step
*       and representable ratios.
*       This does not round incoming samples or alter stored values. No redraw
*       or relayout is requested by the setter alone.
*
*/
/****** Graph.mcc/MUIA_Graph_ManualRefresh ************
*
*   NAME
*       MUIA_Graph_ManualRefresh
*
*   FUNCTION
*       Choose whether construction installs the automatic tick-redraw
*       notification.
*
*   INPUTS
*       Initialize, set; BOOL, default FALSE. No local getter.
*
*   NOTES
*       At the end of construction, FALSE causes a notification on
*       PeriodicTick=TRUE to call an internal redraw hook. TRUE skips
*       installation. The flag is writable later, but setting it does not add or
*       remove that notification. Choose the mode at construction.
*       Manual refresh does not disable sampling. Request drawing through the
*       normal MUI redraw mechanism when needed. InfoText changes can still
*       request redraw directly while shown.
*
*/
/****** Graph.mcc/MUIA_Graph_PeriodicTick ************
*
*   NAME
*       MUIA_Graph_PeriodicTick
*
*   FUNCTION
*       Provide an attribute notification trigger after periodic sampling.
*
*   INPUTS
*       Notification/set tag with BOOL. No stored value or local getter.
*
*   NOTES
*       Timer sends TRUE after its periodic sampling pass. The setter only
*       counts this recognized tag and forwards it; manually setting it does not
*       sample data. The constructor normally installs a TRUE-triggered redraw
*       notification unless ManualRefresh was TRUE.
*       A tick notification is not evidence that every hook supplied a value or
*       that storage allocation succeeded. Zero sources still permits the
*       trigger while periodic mode is enabled. There is no separate accumulated
*       tick count exposed by this attribute.
*
*/
/****** Graph.mcc/MUIM_Graph_GetSourceHandle ************
*
*   NAME
*       MUIM_Graph_GetSourceHandle
*
*   SYNOPSIS
*       APTR handle = (APTR)DoMethod(obj, MUIM_Graph_GetSourceHandle, SourceNo);
*
*   FUNCTION
*       Return an internal source record, growing the source array when needed.
*
*   INPUTS
*       obj - Live Graph object.
*       SourceNo - ULONG, zero-based index. The constructor requests source
*       zero.
*
*   RESULT
*       Borrowed pointer to the requested source record. There is no reliable
*       NULL-on-allocation-failure contract.
*
*   NOTES
*       The handle is opaque to callers and is intended for SetSourceAttrib.
*       Never free it. Any later growth of the source array can invalidate every
*       earlier handle, even when sample buffers remain allocated. Disposal
*       invalidates all handles.
*       Create consecutive indices: growth initializes only the last new record,
*       not skipped indices. Request all needed sources before fetching handles
*       used for configuration. Avoid source growth while shown or while a
*       sampling callback is active. The method does not validate allocation
*       success before forming the returned address.
*
*/
/****** Graph.mcc/MUIM_Graph_SetSourceAttrib ************
*
*   NAME
*       MUIM_Graph_SetSourceAttrib
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Graph_SetSourceAttrib, handle, attrib,
*       value);
*
*   FUNCTION
*       Assign a read hook or pen setting to an existing source record.
*
*   INPUTS
*       handle - Current source handle belonging to this object; not validated.
*       attrib - One of MUIV_Graph_Source_ReadHook, PenSrc, Pen, FillPenSrc, or
*       FillPen.
*       value - IPTR interpreted according to attrib.
*
*   RESULT
*       Always zero, including unknown selectors. No validation or acquisition
*       status is returned.
*
*   NOTES
*       ReadHook stores a borrowed struct Hook pointer; NULL disables reading
*       that source. Timer calls it synchronously with the destination IPTR
*       sample slot as the hook object and hook->h_Data as the message. Write
*       the sample through that destination; the hook return value is ignored.
*       Keep the hook and its data valid, and do not retain the sample-slot
*       pointer or reconfigure/dispose the graph during the callback.
*       PenSrc stores a borrowed pointer to three ULONG RGB components used by
*       Show for the line pen. Each component ranges from zero to 0xffffffff.
*       The array must remain valid across relevant Show calls. Setting it does
*       not immediately reacquire a pen or redraw.
*       Pen and FillPen replace stored WORD pen numbers. If the previous
*       pen number is not -1, release it against the current screen
*       colormap before assigning the replacement. Supply an
*       appropriate acquired reservation for that same screen; the class
*       releases retained pen numbers during Hide. This is not a
*       borrowed-color-index API. These setters require usable screen state when
*       releasing an old pen.
*       FillPenSrc is incorrectly implemented: it assigns the value to the
*       line-pen field instead of the fill RGB source pointer, without releasing
*       the previous line pen. Do not use it to provide RGB storage. No public
*       working fill-RGB-source assignment is implemented here.
*       Hide does not reset source pen fields, and equal line/fill numbers are
*       not released independently. Do not infer arbitrary repeated assignment
*       or lifecycle safety from these low-level selectors. No setter here
*       requests redraw.
*
*/
/****** Graph.mcc/MUIM_Graph_Reset ************
*
*   NAME
*       MUIM_Graph_Reset
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Graph_Reset);
*
*   FUNCTION
*       Accept a reset request without changing graph state.
*
*   INPUTS
*       obj - Live Graph object. No payload arguments.
*
*   RESULT
*       Always zero.
*
*   NOTES
*       The implementation is a TODO stub. It does not clear samples, reset the
*       write position or tick counter, remove sources, change attributes, or
*       redraw. Recreating the object is distinct from calling this method; no
*       reset behavior is implemented.
*
*/
/****** Graph.mcc/MUIM_Graph_Timer ************
*
*   NAME
*       MUIM_Graph_Timer
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Graph_Timer);
*
*   FUNCTION
*       Advance the internal tick phase and, when periodic mode is enabled,
*       sample each hooked source.
*
*   INPUTS
*       obj - Live Graph with valid configured source storage and hooks. No
*       payload arguments.
*
*   RESULT
*       Always zero; sampling and allocation failures are not returned.
*
*   NOTES
*       Add the stored interval to the internal tick counter and subtract
*       PeriodStep once when exceeded. When periodic mode is disabled, stop
*       after that phase update. Manual invocation does not bypass the mode
*       check or measure elapsed time.
*       In periodic mode, shift full fixed histories or grow nonfixed hooked
*       sources as needed, then call each read hook with its destination sample
*       slot and h_Data. Increment the common write position once per pass, not
*       once per hook. Unhooked sources are skipped but still share that
*       position and are still drawn.
*       Send PeriodicTick=TRUE after the pass. Hooks must write a valid sample
*       synchronously. No locking, deferred callback queue, source-lifetime
*       guard, or checked allocation rollback is supplied. Use through the
*       Application GUI lifecycle and do not mutate graph storage during
*       callbacks.
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

/* Resize the owned source-record array when count changes. Copy the common
 * prefix and free the old array only after successful allocation. Growth
 * initializes only the final record and optionally allocates its zeroed
 * sample array. Return the new/current pointer or NULL on allocation
 * failure; neither skipped records nor removed-record resources are fully
 * managed. Existing record handles become stale on successful resize.
 */
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

/* For count greater than the shared recorded capacity, select sourceNo and
 * allocate a larger array. Copy and free the old array using the shared
 * capacity, then install the replacement if allocation succeeded. New tail
 * bytes are uninitialized. Return the source record even on allocation
 * failure, or NULL when no growth was requested. Do not change shared
 * capacity here.
 */
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

/* Remove and free every owned information-text node, resetting the stored
 * line count to zero. Require an initialized list in a live instance. No
 * caller string is freed and no redraw or error result is produced.
 */
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

/* Discard prior information text, then allocate and copy delimited
 * fragments of a readable NUL-terminated input. NULL leaves an empty list.
 * Terminator handling can add empty nodes and consecutive newlines are not
 * consistently split as blank lines. Allocation results are unchecked.
 * Return the stored node count; no rollback or reliable allocation error is
 * provided.
 */
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

/* Sum count readable IPTR entries and divide a positive sum by count,
 * returning an integer mean rather than a source aggregate. A zero sum
 * returns zero; count zero performs no reads. Arithmetic is unchecked.
 * Drawing currently passes span one, so the result is a single sample.
 */
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

/* Receive the installed PeriodicTick notification through the AROS hook ABI
 * and request MADF_DRAWUPDATE for the supplied object. Ignore hook and
 * message arguments. No sampling, allocation, or manual-refresh flag test
 * occurs here; callback lifetime follows the object notification.
 */
AROS_UFH3(
    void, Graph__HookFunc_DrawUpdateFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(Object *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT

    D(bug("[Graph] %s(0x%p)\n", __func__, obj));

    MUI_Redraw(obj, MADF_DRAWUPDATE);

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
/* Create the superclass with default inner margins, initialize
 * text/state/pens, request source zero, and initialize timer defaults with
 * periodic mode off. Apply the original tags through SetAttrsA. Install the
 * tick redraw hook only if automatic refresh is selected at that moment.
 * Return superclass object or NULL; subordinate allocation failures are not
 * comprehensively checked.
 */
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

        /* We always have atleast one source .. */
        Graph__UpdateSourceArray(data, 1);

        data->graph_IHN.ihn_Flags  = MUIIHNF_TIMER;
        data->graph_IHN.ihn_Method = MUIM_Graph_Timer;
        data->graph_IHN.ihn_Object = obj;
        data->graph_IHN.ihn_Millis = 1000;

        data->graph_PeriodCeiling = data->graph_IHN.ihn_Millis * 10;
        data->graph_PeriodStepping = data->graph_IHN.ihn_Millis;

        data->graph_ValCeiling = 100;
        data->graph_ValStepping = 10;

        SetAttrsA(obj, msg->ops_AttrList);

        if (data->graph_Flags & GRAPHF_REDRAWUPDATE)
        {
            D(bug("[Graph] %s: Configuring notification ... \n", __func__);)
            data->graph_DrawUpdateHook.h_Entry = ( HOOKFUNC )Graph__HookFunc_DrawUpdateFunc;
            DoMethod
              (
                obj, MUIM_Notify, MUIA_Graph_PeriodicTick, TRUE,
                (IPTR) obj, 3,
                MUIM_CallHook, &data->graph_DrawUpdateHook, (IPTR)NULL
              );
        }
    }

    return (IPTR)obj;
}

/* Free each recorded sample array using shared EntryCount, then the source
 * array and copied text nodes. Delegate disposal and return its result.
 * Require prior Hide/Cleanup for pens and timer; this method does not
 * independently release those resources or repair mismatched capacities.
 */
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


/* Process recognized tags sequentially and count them. Interval can issue a
 * nested EntryCount set and register/remove the timer. Text alone marks
 * direct redraw while shown. Forward original tags and return recognized
 * count plus superclass result. Allocation failures are not represented by
 * that total and no change is transactional.
 */
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

            /* Aggreagte mode plots the sum of source entries/no of sources */
            case MUIA_Graph_Aggregate:
                data->graph_Flags &= ~GRAPHF_AGGR;
                if (tag->ti_Data)
                    data->graph_Flags |= GRAPHF_AGGR;
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
                data->graph_ValCeiling = tag->ti_Data;
                count++;
                break;

            case MUIA_Graph_ValueStep:
                data->graph_ValStepping = tag->ti_Data;
                count++;
                break;

            /* Set the period ceiling and stepping */
            case MUIA_Graph_PeriodCeiling:
                data->graph_PeriodCeiling = tag->ti_Data;
                count++;
                break;

            case MUIA_Graph_PeriodStep:
                data->graph_PeriodStepping = tag->ti_Data;
                count++;
                break;

            /* Set or turn off periodic update mode */
            case MUIA_Graph_PeriodInterval:
                if (tag->ti_Data)
                {
                    data->graph_Flags |= GRAPHF_PERIODIC;
                    data->graph_IHN.ihn_Millis = tag->ti_Data;
                    SET(obj, MUIA_Graph_EntryCount, (data->graph_PeriodCeiling/ data->graph_IHN.ihn_Millis));
                    if ((data->graph_Flags & GRAPHF_SETUP) && !(data->graph_Flags & GRAPHF_HANDLER))
                    {
                        data->graph_Flags |= GRAPHF_HANDLER;
                        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->graph_IHN);
                    }
                }
                else
                {
                    data->graph_Flags &= ~GRAPHF_PERIODIC;
                    if ((data->graph_Flags & GRAPHF_SETUP) && (data->graph_Flags & GRAPHF_HANDLER))
                    {
                        DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->graph_IHN);
                        data->graph_Flags &= ~GRAPHF_HANDLER;
                    }
                }
                count++;
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


/* Return ValueCeiling, stored EntryCount, or stored PeriodInterval with
 * TRUE; delegate all other attributes. Counts do not describe initialized
 * sample population, and the interval remains stored after disabling
 * periodic mode. No other Graph attribute has a local getter.
 */
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
            *(msg->opg_Storage) = data->graph_IHN.ihn_Millis;
            break;

        default:
            retval = DoSuperMethodA(cl, obj, (Msg)msg);
            break;
    }
    
    return retval;
}

/* Set up the superclass, returning FALSE on its failure. Register the
 * Application timer if periodic and not already marked registered, then set
 * the setup flag. Return TRUE without checking registration success.
 * Hidden-state transitions do not own this timer.
 */
IPTR Graph__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    D(bug("[Graph] %s()\n", __func__);)

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    if ((data->graph_Flags & GRAPHF_PERIODIC) && !(data->graph_Flags & GRAPHF_HANDLER))
    {
        data->graph_Flags |= GRAPHF_HANDLER;
        DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR) &data->graph_IHN);
    }

    data->graph_Flags |= GRAPHF_SETUP;

    return TRUE;
}


/* Show the superclass and return FALSE on its failure. Obtain
 * background/axis/grid pens and source pens from borrowed RGB arrays, then
 * mark shown. Acquisition failure does not make Show fail. Equal line/fill
 * source pointers skip fill acquisition without assigning a shared pen
 * number. Return TRUE.
 */
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

    data->graph_Flags |= GRAPHF_SHOW;

    return TRUE;
}

/* Clear shown, release background/axis/grid pens and reset their sentinels.
 * Release source fill pens only when distinct from line pens, then release
 * line pens; source fields remain unchanged. Delegate Hide. This does not
 * unregister the sampling timer or reconcile independent equal-index
 * reservations.
 */
IPTR Graph__MUIM_Hide(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);
    int i;

    D(bug("[Graph] %s()\n", __func__);)

    data->graph_Flags &= ~GRAPHF_SHOW;

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

/* Clear setup and remove the timer only when both periodic and handler
 * flags are set, then clear the handler flag. Delegate Cleanup and return
 * its result. Screen pens belong to Hide and are not released here.
 */
IPTR Graph__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct Graph_DATA *data = INST_DATA(cl, obj);

    D(bug("[Graph] %s()\n", __func__);)

    data->graph_Flags &= ~GRAPHF_SETUP;

    if ((data->graph_Flags & GRAPHF_PERIODIC) && (data->graph_Flags & GRAPHF_HANDLER))
    {
        DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR) &data->graph_IHN);
        data->graph_Flags &= ~GRAPHF_HANDLER;
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/* Delegate measurement, then add two pixels per ceiling/step division to
 * minima and ten to defaults, using period for width and value for height.
 * Set maxima MUI_MAXMAX and return TRUE. Require nonzero steps and
 * representable products; there is no validation or text-size contribution.
 */
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

/* Add a clip region when available, call the superclass, and render
 * relevant object/update requests. Draw filled series, divisions, frame,
 * line series, and centered text using configured scales and source
 * storage. Temporary fill buffers are freed on completed allocation paths.
 * Remove the added clip and return zero. Allocation failures can skip fill
 * without an error; usable arrays, positive scale divisors and valid pens
 * remain preconditions.
 */
IPTR Graph__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Graph_DATA           *data = INST_DATA(cl, obj);
    struct Graph_SourceDATA     *sourceData;
    struct Region               *region;
    struct Node                 *infoLine;
    struct RastPort             *renderPort;
    struct Rectangle            rect;
    APTR                        clip = NULL;
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

        data->graph_PeriodSize = ((float)objWidth / data->graph_PeriodCeiling) * data->graph_IHN.ihn_Millis;

        if (data->graph_Flags & GRAPHF_PERIODIC)
            offset = data->graph_Tick / data->graph_IHN.ihn_Millis;

        D(
            bug("[Graph] %s: dimensions %d,%d\n", __func__, objWidth, objHeight);
            bug("[Graph] %s: period size %d, span %d\n", __func__, (int)data->graph_PeriodSize, span);
            bug("[Graph] %s: offset %d\n", __func__, offset);
        )

        // First fill the background ..
        if (data->graph_Flags & GRAPHF_CLEAR)
        {
            if (data->graph_BackPen != -1)
                SetAPen(renderPort, data->graph_BackPen);
            RectFill(renderPort, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
        }

        // fill the Area if wanted ...
        if ((data->graph_Flags & GRAPHF_FILL) && (data->graph_Sources))
        {
            struct RastPort *drp;
            if ((drp = CloneRastPort(renderPort)))
            {
                IPTR dispDepth = GetCyberMapAttr(drp->BitMap, CYBRMATTR_DEPTH);
                APTR tmpbuf;

                if ((tmpbuf = (APTR)AllocMem(objWidth * objHeight * dispDepth, MEMF_CHIP | MEMF_CLEAR)))
                {
                    APTR vectors;
                    if ((vectors = AllocVec((data->graph_EntryCount + 5) * 5, MEMF_ANY | MEMF_CLEAR)))
                    {
                        struct AreaInfo fAInfo = {0};
                        struct TmpRas fRasTmp;
                        UWORD dither[] =
                        {
                            0x5555, 0xAAAA
                        };

                        D(bug("[Graph] %s: Filling area ...\n", __func__);)

                        drp->AreaInfo = &fAInfo;

                        InitTmpRas(&fRasTmp, tmpbuf, objWidth * objHeight * dispDepth);
                        drp->TmpRas = &fRasTmp;

                        InitArea( &fAInfo, vectors, data->graph_EntryCount  + 4);

                        for (src = 0; src < data->graph_SourceCount; src ++)
                        {
                            UWORD xpos, ypos, lastpos;
                            LONG start;

                            if (data->graph_Sources[src].gs_PlotFillPen != -1)
                                SetAPen(drp, data->graph_Sources[src].gs_PlotFillPen);
                            else
                            {
                                SetAPen(drp, data->graph_Sources[src].gs_PlotPen);
                                SetAfPt(drp, dither, 1);
                            }

                            sourceData = &data->graph_Sources[src];

                            if ((start = data->graph_EntryPtr - (objWidth / data->graph_PeriodSize)) < 0)
                                start = 0;

                            ypos = (objHeight * Graph__SumValues(&sourceData->gs_Entries[start], span))/ data->graph_ValCeiling;
                            start += span;

                            if (data->graph_Flags & GRAPHF_PERIODIC)
                                xpos = rect.MaxX - (data->graph_EntryPtr * data->graph_PeriodSize);
                            else
                                xpos = rect.MinX - (offset * data->graph_PeriodSize);

                            AreaMove(drp, xpos, rect.MaxY);
                            AreaDraw(drp, xpos, rect.MaxY - ypos);

                            lastpos = start;
                            for (pos = start; pos < data->graph_EntryPtr; pos += span)
                            {
                                ypos = (objHeight * Graph__SumValues(&sourceData->gs_Entries[pos], span))/ data->graph_ValCeiling;

                                AreaDraw(drp,
                                    xpos + (pos * data->graph_PeriodSize),
                                    rect.MaxY - ypos);
                                lastpos = pos;
                            }
                            AreaDraw(drp,
                                    xpos + (lastpos * data->graph_PeriodSize),
                                    rect.MaxY);
                            AreaDraw(drp,
                                 xpos, rect.MaxY);
                            AreaEnd(drp);
                        }
                        FreeVec(vectors);
                    }
                    FreeMem(tmpbuf, objWidth * objHeight * dispDepth);
                }
                drp->AreaInfo = NULL;
                drp->TmpRas = NULL;
                FreeRastPort(drp);
            }
        }

        if (data->graph_Flags & GRAPHF_DRAWSEGS)
        {
            // Draw the segment divisions..
            if (data->graph_SegmentPen != -1)
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
            if (data->graph_AxisPen != -1)
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

                SetAPen(renderPort, data->graph_Sources[src].gs_PlotPen);
                sourceData = &data->graph_Sources[src];

                if ((start = data->graph_EntryPtr - (objWidth / data->graph_PeriodSize)) < 0)
                    start = 0;

                ypos = (objHeight * Graph__SumValues(&sourceData->gs_Entries[start], span))/ data->graph_ValCeiling;
                start += span;

                if (data->graph_Flags & GRAPHF_PERIODIC)
                    xpos = rect.MaxX - (data->graph_EntryPtr * data->graph_PeriodSize);
                else
                    xpos = rect.MinX - (offset * data->graph_PeriodSize);

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

        SetAPen(renderPort, _pens(obj)[MPEN_TEXT]);
        ForeachNode(&data->graph_InfoText, infoLine)
        {
            UWORD txtLen = strlen(infoLine->ln_Name);
            UWORD textWidth = TextLength(renderPort, infoLine->ln_Name, txtLen);

            D(bug("[Graph] %s: pos = %d, strlen = %d, wid = %d\n", __func__, pos, txtLen, textWidth);)

            if (textWidth > 0)
            {
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

/* Grow the source array to SourceNo+1 when needed, then return the indexed
 * internal record address. Do not check growth success or validate
 * intermediate initialization. The borrowed handle is invalidated by later
 * successful array reallocation or disposal; it is not a stable external
 * identifier.
 */
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

/* Assign the selected hook, RGB pointer, or WORD pen directly through an
 * unchecked source handle. Pen replacements release prior reservations
 * using the current screen. FillPenSrc incorrectly writes the line-pen
 * field. Return zero for all selectors without requesting redraw or
 * reporting failure.
 */
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

/* Return zero without changing any instance state. This TODO method does
 * not clear history, rewind indices, free sources, stop timers, or request
 * drawing; callers must not interpret its return as successful reset.
 */
IPTR Graph__MUIM_Graph_Reset(Class *cl, Object *obj, Msg msg)
{
    D(bug("[Graph] %s()\n", __func__);)

    //TODO: Implement Graph Reset

    return 0;
}

/* Advance tick phase by the stored interval, with a single strict-greater-
 * than step subtraction. If periodic, prepare fixed/nonfixed slots, call
 * each read hook synchronously, advance shared position and send
 * PeriodicTick TRUE. Storage growth failures are ignored. Return zero;
 * require stable source/storage lifetimes throughout callbacks.
 */
IPTR Graph__MUIM_Graph_Timer(Class *cl, Object *obj, Msg msg)
{
    struct Graph_DATA *data;
    int i;

    D(bug("[Graph] %s()\n", __func__);)

    data = INST_DATA(cl, obj);

    if ((data->graph_Tick += data->graph_IHN.ihn_Millis) > data->graph_PeriodStepping)
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
