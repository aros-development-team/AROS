/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Flexgroup: a Group that lays its children out like a CSS flexbox.

    The class owns a MUIA_Group_LayoutHook and drives the standard
    MUILM_MINMAX / MUILM_LAYOUT protocol from it, so no change to Group
    itself is needed.

    Implemented: direction, gap, justify-content, align-items, flex-grow,
    flex-shrink and flex-basis.  flex-wrap is not implemented - AskMinMax
    can only report a scalar MinHeight that is independent of the width,
    so wrapping needs either a virtual group or an extra layout pass.
*/

#include <exec/types.h>
#include <clib/macros.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <aros/debug.h>

#include "flexgroup.h"
#include "flexgroup_private.h"

/* The attributes we own, in the order flex_attr_slot() numbers them. */
static const Tag flex_attrs[] =
{
    MUIA_Flexgroup_Direction,
    MUIA_Flexgroup_Justify,
    MUIA_Flexgroup_Align,
    MUIA_Flexgroup_Gap
};
#define FLEX_ATTRS (sizeof(flex_attrs) / sizeof(flex_attrs[0]))

static struct Flexgroup_DATA *flex_data(struct Hook *hook)
{
    return (struct Flexgroup_DATA *)hook;
}

/* Collects visible children into the cached table.  Returns the count, or
   -1 if the table could not be grown. */
static LONG flex_gather(struct Flexgroup_DATA *data, struct MinList *children)
{
    struct FlexCalc *calc;
    Object *cstate, *child;
    LONG n = 0, i = 0;

    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)))
    {
        if (XGET(child, MUIA_ShowMe))
            n++;
    }
    if (n == 0)
        return 0;

    if (n > data->calcsize)
    {
        FreeVec(data->calc);
        data->calc = AllocVec(n * sizeof(struct FlexCalc), MEMF_ANY);
        data->calcsize = (data->calc != NULL) ? n : 0;
        if (data->calc == NULL)
            return -1;
    }
    calc = data->calc;

    cstate = (Object *)children->mlh_Head;
    while ((child = NextObject(&cstate)) && i < n)
    {
        struct FlexCalc *c = &calc[i];

        if (!XGET(child, MUIA_ShowMe))
            continue;

        c->child = child;
        c->frozen = FALSE;

        if (data->direction == MUIV_Flexgroup_Direction_Row)
        {
            c->min = _minwidth(child);
            c->def = _defwidth(child);
            c->max = _maxwidth(child);
            c->cmin = _minheight(child);
            c->cdef = _defheight(child);
            c->cmax = _maxheight(child);
            c->weight = XGET(child, MUIA_HorizWeight);
        }
        else
        {
            c->min = _minheight(child);
            c->def = _defheight(child);
            c->max = _maxheight(child);
            c->cmin = _minwidth(child);
            c->cdef = _defwidth(child);
            c->cmax = _maxwidth(child);
            c->weight = XGET(child, MUIA_VertWeight);
        }

        c->basis = CLAMP(c->def, c->min, c->max);
        i++;
    }

    return i;
}

static void flex_minmax(struct Flexgroup_DATA *data,
    struct MUI_LayoutMsg *lm)
{
    struct FlexCalc *calc;
    LONG n, i, gaps;
    LONG minmain = 0, defmain = 0, maxmain;
    LONG mincross = 0, defcross = 0, maxcross = MUI_MAXMAX;

    lm->lm_MinMax.MinWidth = lm->lm_MinMax.DefWidth = 0;
    lm->lm_MinMax.MinHeight = lm->lm_MinMax.DefHeight = 0;
    lm->lm_MinMax.MaxWidth = lm->lm_MinMax.MaxHeight = MUI_MAXMAX;

    n = flex_gather(data, lm->lm_Children);
    if (n <= 0)
        return;
    calc = data->calc;

    gaps = (n - 1) * data->gap;

    for (i = 0; i < n; i++)
    {
        struct FlexCalc *c = &calc[i];

        /* A weight of 0 can neither grow nor shrink, so its natural size
           is also its minimum. */
        minmain += (c->weight > 0) ? c->min : c->basis;
        defmain += c->basis;

        mincross = MAX(mincross, c->cmin);
        defcross = MAX(defcross, c->cdef);
        if (data->align == MUIV_Flexgroup_Align_Stretch)
            maxcross = MIN(maxcross, c->cmax);
    }

    minmain += gaps;
    defmain += gaps;

    /* A flex container takes its size from its parent, not from the sum
       of its children - justify-content exists precisely to distribute
       the surplus.  Reporting sum(basis) here would make group.c hand us
       exactly the content size and leave nothing to justify. */
    maxmain = MUI_MAXMAX;

    /* Same reasoning on the cross axis: without stretch no child caps
       us, and without headroom there is nothing to align within. */
    if (data->align != MUIV_Flexgroup_Align_Stretch)
        maxcross = MUI_MAXMAX;

    maxcross = MIN(MAX(maxcross, mincross), MUI_MAXMAX);
    defmain = CLAMP(defmain, minmain, maxmain);
    defcross = CLAMP(defcross, mincross, maxcross);

    if (data->direction == MUIV_Flexgroup_Direction_Row)
    {
        lm->lm_MinMax.MinWidth = minmain;
        lm->lm_MinMax.DefWidth = defmain;
        lm->lm_MinMax.MaxWidth = maxmain;
        lm->lm_MinMax.MinHeight = mincross;
        lm->lm_MinMax.DefHeight = defcross;
        lm->lm_MinMax.MaxHeight = maxcross;
    }
    else
    {
        lm->lm_MinMax.MinHeight = minmain;
        lm->lm_MinMax.DefHeight = defmain;
        lm->lm_MinMax.MaxHeight = maxmain;
        lm->lm_MinMax.MinWidth = mincross;
        lm->lm_MinMax.DefWidth = defcross;
        lm->lm_MinMax.MaxWidth = maxcross;
    }
}

/* Hands out surplus in proportion to weight, saturating at ->max. */
static void flex_grow(struct FlexCalc *calc, LONG n, LONG freespace)
{
    LONG pass, i;

    for (pass = 0; pass < 8 && freespace > 0; pass++)
    {
        LONG wsum = 0, used = 0;

        for (i = 0; i < n; i++)
        {
            if (!calc[i].frozen && calc[i].weight > 0)
                wsum += calc[i].weight;
        }
        if (wsum == 0)
            return;

        for (i = 0; i < n; i++)
        {
            struct FlexCalc *c = &calc[i];
            LONG add;

            if (c->frozen || c->weight <= 0)
                continue;

            add = (freespace * c->weight) / wsum;
            if (c->size + add >= c->max)
            {
                used += c->max - c->size;
                c->size = c->max;
                c->frozen = TRUE;
            }
            else
            {
                used += add;
                c->size += add;
            }
        }
        if (used == 0)
            break;
        freespace -= used;
    }

    /* Rounding remainder, one pixel at a time. */
    while (freespace > 0)
    {
        BOOL progress = FALSE;

        for (i = 0; i < n && freespace > 0; i++)
        {
            if (calc[i].weight > 0 && calc[i].size < calc[i].max)
            {
                calc[i].size++;
                freespace--;
                progress = TRUE;
            }
        }
        if (!progress)
            break;
    }
}

/* Takes back a deficit in proportion to weight * basis, as CSS does,
   saturating at ->min. */
static void flex_shrink(struct FlexCalc *calc, LONG n, LONG deficit)
{
    LONG pass, i;

    /* deficit * weight * basis overflows a LONG long before the weights
       themselves do, so keep the ratio in 64 bits. */
    for (pass = 0; pass < 8 && deficit > 0; pass++)
    {
        UQUAD wsum = 0;
        LONG used = 0;

        for (i = 0; i < n; i++)
        {
            if (!calc[i].frozen && calc[i].weight > 0)
                wsum += (UQUAD) calc[i].weight * calc[i].basis;
        }
        if (wsum == 0)
            return;

        for (i = 0; i < n; i++)
        {
            struct FlexCalc *c = &calc[i];
            LONG sub;

            if (c->frozen || c->weight <= 0)
                continue;

            sub = (LONG) ((UQUAD) deficit * c->weight * c->basis / wsum);
            if (c->size - sub <= c->min)
            {
                used += c->size - c->min;
                c->size = c->min;
                c->frozen = TRUE;
            }
            else
            {
                used += sub;
                c->size -= sub;
            }
        }
        if (used == 0)
            break;
        deficit -= used;
    }

    while (deficit > 0)
    {
        BOOL progress = FALSE;

        for (i = 0; i < n && deficit > 0; i++)
        {
            if (calc[i].weight > 0 && calc[i].size > calc[i].min)
            {
                calc[i].size--;
                deficit--;
                progress = TRUE;
            }
        }
        if (!progress)
            break;
    }
}

static ULONG flex_layout(struct Flexgroup_DATA *data,
    struct MUI_LayoutMsg *lm)
{
    struct FlexCalc *calc;
    LONG n, i, gaps, content, total = 0, slack, pos, between;
    LONG mainavail, crossavail;

    n = flex_gather(data, lm->lm_Children);
    if (n < 0)
        return FALSE;
    if (n == 0)
        return TRUE;
    calc = data->calc;

    if (data->direction == MUIV_Flexgroup_Direction_Row)
    {
        mainavail = lm->lm_Layout.Width;
        crossavail = lm->lm_Layout.Height;
    }
    else
    {
        mainavail = lm->lm_Layout.Height;
        crossavail = lm->lm_Layout.Width;
    }

    gaps = (n - 1) * data->gap;
    content = mainavail - gaps;

    for (i = 0; i < n; i++)
    {
        calc[i].size = calc[i].basis;
        total += calc[i].size;
    }

    if (total < content)
        flex_grow(calc, n, content - total);
    else if (total > content)
        flex_shrink(calc, n, total - content);

    total = 0;
    for (i = 0; i < n; i++)
        total += calc[i].size;

    slack = mainavail - total - gaps;
    if (slack < 0)
        slack = 0;

    D(bug("[Flexgroup] dir=%d justify=%d align=%d gap=%d avail=%ld "
            "content=%ld gaps=%ld slack=%ld\n", data->direction,
            data->justify, data->align, data->gap, mainavail, total, gaps,
            slack));

    pos = 0;
    between = data->gap;
    switch (data->justify)
    {
    case MUIV_Flexgroup_Justify_End:
        pos = slack;
        break;
    case MUIV_Flexgroup_Justify_Center:
        pos = slack / 2;
        break;
    case MUIV_Flexgroup_Justify_SpaceBetween:
        if (n > 1)
            between += slack / (n - 1);
        break;
    case MUIV_Flexgroup_Justify_SpaceAround:
        pos = slack / (n * 2);
        between += (slack / (n * 2)) * 2;
        break;
    case MUIV_Flexgroup_Justify_SpaceEvenly:
        pos = slack / (n + 1);
        between += slack / (n + 1);
        break;
    }

    for (i = 0; i < n; i++)
    {
        struct FlexCalc *c = &calc[i];
        LONG csize, cpos;

        if (data->align == MUIV_Flexgroup_Align_Stretch)
        {
            csize = CLAMP(crossavail, c->cmin, c->cmax);
            cpos = 0;
        }
        else
        {
            csize = CLAMP(c->cdef, c->cmin, c->cmax);
            switch (data->align)
            {
            case MUIV_Flexgroup_Align_End:
                cpos = crossavail - csize;
                break;
            case MUIV_Flexgroup_Align_Center:
                cpos = (crossavail - csize) / 2;
                break;
            default:
                cpos = 0;
                break;
            }
            if (cpos < 0)
                cpos = 0;
        }

        if (data->direction == MUIV_Flexgroup_Direction_Row)
            MUI_Layout(c->child, pos, cpos, c->size, csize, 0);
        else
            MUI_Layout(c->child, cpos, pos, csize, c->size, 0);

        pos += c->size + between;
    }

    return TRUE;
}

static ULONG Flexgroup_LayoutFunc(struct Hook *hook, Object *obj,
    struct MUI_LayoutMsg *lm)
{
    struct Flexgroup_DATA *data = flex_data(hook);

    switch (lm->lm_Type)
    {
    case MUILM_MINMAX:
        flex_minmax(data, lm);
        return 0;

    case MUILM_LAYOUT:
        return flex_layout(data, lm);
    }

    return MUILM_UNKNOWN;
}

/* Puts our own hook back in place, bypassing our OM_SET so we do not
   have to filter the caller's tag list.  Forwarding has to be off, or
   Group hands the hook to every child group as well. */
static void flex_claim_hook(struct IClass *cl, Object *obj,
    struct Flexgroup_DATA *data)
{
    struct TagItem tags[3];

    tags[0].ti_Tag = MUIA_Group_LayoutHook;
    tags[0].ti_Data = (IPTR) & data->hook;
    tags[1].ti_Tag = MUIA_Group_Forward;
    tags[1].ti_Data = (IPTR) FALSE;
    tags[2].ti_Tag = TAG_DONE;
    tags[2].ti_Data = 0;

    DoSuperMethod(cl, obj, OM_SET, (IPTR) tags, (IPTR) NULL);
}

IPTR Flexgroup__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Flexgroup_DATA *data;

    if (GetTagData(MUIA_Group_LayoutHook, 0, msg->ops_AttrList))
    {
        bug("Flexgroup: MUIA_Group_LayoutHook ignored, the class owns "
            "the layout hook\n");
    }

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        TAG_MORE, (IPTR) msg->ops_AttrList);
    if (obj == NULL)
        return (IPTR) NULL;

    data = INST_DATA(cl, obj);
    data->hook.h_Entry = (HOOKFUNC) HookEntry;
    data->hook.h_SubEntry = (HOOKFUNC) Flexgroup_LayoutFunc;

    data->direction = GetTagData(MUIA_Flexgroup_Direction,
        MUIV_Flexgroup_Direction_Row, msg->ops_AttrList);
    data->justify = GetTagData(MUIA_Flexgroup_Justify,
        MUIV_Flexgroup_Justify_Start, msg->ops_AttrList);
    data->align = GetTagData(MUIA_Flexgroup_Align,
        MUIV_Flexgroup_Align_Stretch, msg->ops_AttrList);
    data->gap = GetTagData(MUIA_Flexgroup_Gap, 0, msg->ops_AttrList);

    flex_claim_hook(cl, obj, data);

    return (IPTR) obj;
}

IPTR Flexgroup__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Flexgroup_DATA *data = INST_DATA(cl, obj);

    FreeVec(data->calc);

    return DoSuperMethodA(cl, obj, msg);
}

IPTR Flexgroup__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Flexgroup_DATA *data = INST_DATA(cl, obj);
    struct TagItem *mine[FLEX_ATTRS] = { NULL, NULL, NULL, NULL };
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;
    BOOL stolen = FALSE;
    BOOL relayout = FALSE;
    IPTR retval;
    ULONG i;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Flexgroup_Direction:
            data->direction = tag->ti_Data;
            mine[0] = tag;
            break;

        case MUIA_Flexgroup_Justify:
            data->justify = tag->ti_Data;
            mine[1] = tag;
            break;

        case MUIA_Flexgroup_Align:
            data->align = tag->ti_Data;
            mine[2] = tag;
            break;

        case MUIA_Flexgroup_Gap:
            data->gap = tag->ti_Data;
            mine[3] = tag;
            break;

        case MUIA_Group_LayoutHook:
            /* Group honours this in OM_SET too, despite the docs. */
            stolen = TRUE;
            break;
        }
    }

    /* Hide our attributes from the superclass - Group forwards anything
       it does not recognise to the children, and a nested Flexgroup
       would happily adopt them. */
    for (i = 0; i < FLEX_ATTRS; i++)
    {
        if (mine[i] != NULL)
        {
            mine[i]->ti_Tag = TAG_IGNORE;
            relayout = TRUE;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    for (i = 0; i < FLEX_ATTRS; i++)
    {
        if (mine[i] != NULL)
            mine[i]->ti_Tag = flex_attrs[i];
    }

    if (stolen)
    {
        bug("Flexgroup: MUIA_Group_LayoutHook ignored, the class owns "
            "the layout hook\n");
        flex_claim_hook(cl, obj, data);
    }

    if (relayout)
    {
        struct TagItem ntags[FLEX_ATTRS + 2];
        ULONG n = 0;

        /* Second pass with forwarding off, so Notify sees the change and
           MUIM_Notify on our attributes works, but the children still do
           not get them. */
        for (i = 0; i < FLEX_ATTRS; i++)
        {
            if (mine[i] != NULL)
            {
                ntags[n].ti_Tag = flex_attrs[i];
                ntags[n].ti_Data = mine[i]->ti_Data;
                n++;
            }
        }
        ntags[n].ti_Tag = MUIA_Group_Forward;
        ntags[n].ti_Data = (IPTR) FALSE;
        ntags[n + 1].ti_Tag = TAG_DONE;
        ntags[n + 1].ti_Data = 0;

        DoSuperMethod(cl, obj, OM_SET, (IPTR) ntags, (IPTR) NULL);

        if (muiRenderInfo(obj) != NULL)
        {
            DoMethod(obj, MUIM_Group_InitChange);
            DoMethod(obj, MUIM_Group_ExitChange);
        }
    }

    return retval;
}

IPTR Flexgroup__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)
    struct Flexgroup_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Flexgroup_Direction:
        STORE = data->direction;
        return TRUE;
    case MUIA_Flexgroup_Justify:
        STORE = data->justify;
        return TRUE;
    case MUIA_Flexgroup_Align:
        STORE = data->align;
        return TRUE;
    case MUIA_Flexgroup_Gap:
        STORE = data->gap;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}
