/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

*/

/****** Flexgroup.mcc/MUIC_Flexgroup ************
*
*   NAME
*       MUIC_Flexgroup
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Arrange visible children in one row or column using their existing MUI
*       size limits and weights. The class installs its own Group layout hook
*       and implements the MUILM_MINMAX and MUILM_LAYOUT protocol. It does not
*       implement wrapping or a CSS property interface. The source configuration
*       declares version 1.0.
*       Only children with MUIA_ShowMe true participate. A child's starting
*       main-axis size is its default size clamped between minimum and maximum.
*       Positive weights share extra space in proportion to weight and shrink in
*       proportion to weight times starting size, within those limits. Zero
*       weight keeps the starting size. Use MUIA_HorizWeight for Row and
*       MUIA_VertWeight for Column.
*       Gap reserves space between adjacent visible children. Justify
*       distributes any surplus left after growth. Align controls the cross
*       axis. Main-axis maximum is MUI_MAXMAX; Stretch can constrain the
*       cross-axis maximum to child maxima, but never below the largest child
*       minimum. Empty groups report zero minimum/default content sizes and
*       MUI_MAXMAX maxima.
*       The class owns a reusable child-calculation table, grows it when needed,
*       and frees it at disposal. Group retains normal ownership and lifecycle
*       of member objects.
*
*   NOTES
*       Defaults are Direction_Row, Justify_Start, Align_Stretch, and Gap zero.
*       Construction uses the first matching tag for each attribute; Set stores
*       the last occurrence. Values are converted to the private UBYTE selectors
*       or WORD gap without validation.
*       Set temporarily changes the last tag of each Flexgroup attribute to
*       TAG_IGNORE before forwarding to Group, then restores those tags. Supply
*       writable tag lists. A second superclass call sends the selected
*       attributes with Group_Forward FALSE, in Direction/Justify/Align/Gap
*       order, so the normal case notifies this object without setting child
*       Flexgroups. The notification carries the supplied value, which can
*       differ from the converted getter value. Relayout uses
*       InitChange/ExitChange when render information exists, even for unchanged
*       values.
*
*   BUGS
*       Only the last occurrence of each Flexgroup attribute is hidden in the
*       first pass. Earlier duplicates can notify and reach children through
*       Group forwarding. Avoid duplicate Flexgroup tags in Set calls. The
*       second notification pass does not carry the caller's NoNotify tag, so
*       NoNotify does not reliably suppress these attribute notifications.
*       The class reclaims its layout hook after a caller attempts to set
*       Group_LayoutHook. The caller hook still passes through Group first and
*       can reach child groups. Do not supply an independent Group_LayoutHook.
*       Insufficient space does not force children below minima; layout can
*       exceed the available area. Individual MUI_Layout results are ignored.
*       Calculation-table allocation failure makes layout return FALSE, but
*       measurement leaves empty-group bounds and reports no separate error. Use
*       consistent child limits, nonnegative weights, and sizes whose
*       calculations fit the implementation's integer types.
*
*/
/****** Flexgroup.mcc/MUIA_Flexgroup_Direction ************
*
*   NAME
*       MUIA_Flexgroup_Direction
*
*   FUNCTION
*       Select the main axis used for child sizing and placement.
*
*   INPUTS
*       Initialize, set, get; LONG. MUIV_Flexgroup_Direction_Row (0, default) or
*       Column (1).
*
*   NOTES
*       Row lays out left to right and uses horizontal weights; Column lays out
*       top to bottom and uses vertical weights. Child order is preserved;
*       reverse directions and wrapping are not implemented. Stored as UBYTE.
*       Any stored value other than Row takes the column path; unsupported input
*       is not rejected. See the class notes for setter notification and
*       tag-list behavior.
*
*/
/****** Flexgroup.mcc/MUIA_Flexgroup_Justify ************
*
*   NAME
*       MUIA_Flexgroup_Justify
*
*   FUNCTION
*       Distribute unused main-axis space after weighted sizing.
*
*   INPUTS
*       Initialize, set, get; LONG. MUIV_Flexgroup_Justify_Start (0, default),
*       End (1), Center (2), SpaceBetween (3), SpaceAround (4), or SpaceEvenly
*       (5).
*
*   NOTES
*       Start leaves surplus after the children. End puts it before them; Center
*       uses half before. SpaceBetween adds surplus divided by n-1 to each gap
*       when n>1. SpaceAround starts at surplus/(2*n) and adds twice that value
*       between children. SpaceEvenly uses surplus/(n+1) before and between
*       children. All divisions use integers; remaining pixels need not be
*       symmetric.
*       Existing Gap is included in every interior interval. For a single child,
*       SpaceBetween behaves like Start. Growth may consume all surplus, leaving
*       nothing to justify. Stored as UBYTE; an unrecognized stored value
*       behaves like Start. See the class notes for setter behavior.
*
*/
/****** Flexgroup.mcc/MUIA_Flexgroup_Align ************
*
*   NAME
*       MUIA_Flexgroup_Align
*
*   FUNCTION
*       Size and position children across the main axis.
*
*   INPUTS
*       Initialize, set, get; LONG. MUIV_Flexgroup_Align_Stretch (0, default),
*       Start (1), End (2), or Center (3).
*
*   NOTES
*       Stretch clamps available cross-axis size to each child's minimum/maximum
*       and places it at the start. Other modes use the child default clamped to
*       its limits. End offsets by available minus child size; Center uses half
*       that difference; negative offsets are clamped to zero.
*       Stretch also constrains reported cross-axis maximum using child maxima;
*       other modes leave it MUI_MAXMAX. Stored as UBYTE. Unrecognized stored
*       values use non-stretch sizing at the start. Oversized children can
*       extend beyond the group; no additional shrink-to-fit rule applies.
*
*/
/****** Flexgroup.mcc/MUIA_Flexgroup_Gap ************
*
*   NAME
*       MUIA_Flexgroup_Gap
*
*   FUNCTION
*       Set the baseline spacing between adjacent visible children.
*
*   INPUTS
*       Initialize, set, get; LONG converted to signed WORD. Default zero. Use 0
*       through 32767 for nonnegative spacing.
*
*   NOTES
*       For n visible children the reserved amount is (n-1)*Gap. Hidden children
*       contribute neither size nor gaps. Justify can add space to this
*       baseline; there is no separately requested outer gap.
*       Negative values are accepted and can overlap children; the class does
*       not reject or clamp them. Out-of-range input is converted to WORD. Get
*       returns the stored value, not the original tag value. Gap affects both
*       measurement and placement; see the class notes for relayout and
*       notification behavior.
*
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

/* Recover instance storage from the address of its first member, hook.
 * Require a hook embedded at offset zero in a live Flexgroup_DATA; no
 * validation or ownership transfer occurs. Return the enclosing instance
 * pointer.
 */
static struct Flexgroup_DATA *flex_data(struct Hook *hook)
{
    return (struct Flexgroup_DATA *)hook;
}

/* Collects visible children into the cached table.  Returns the count, or
   -1 if the table could not be grown. */
/* Count ShowMe children and collect their borrowed object pointers,
 * main/cross size limits and axis weights. Grow the owned calculation table
 * if necessary, freeing the old one first. Return count, zero for no
 * visible children, or -1 on allocation failure. Reset frozen and clamp
 * each basis to its limits; caller must serialize child-list changes.
 */
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

/* Initialize empty-content bounds, then gather visible children. On no
 * children or allocation failure, leave those bounds. Sum main minima or
 * fixed bases and gaps, take largest cross minima/defaults, and constrain
 * cross maximum only for Stretch. Map axes into the hook message. No
 * separate error return exists.
 */
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
/* Distribute positive free space among positive-weight unfrozen entries,
 * respecting maximum sizes. Use up to eight proportional passes, then
 * assign remaining pixels in child order while progress is possible. Modify
 * size/frozen in place. Integer arithmetic must remain representable;
 * unused surplus is allowed when all eligible children saturate.
 */
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
/* Remove a positive deficit in proportion to positive weight times basis,
 * respecting minima. Use 64-bit ratios for proportional passes, then remove
 * pixels in child order while progress is possible. Modify size/frozen in
 * place; return without consuming the deficit when the active weight sum is
 * zero. Zero-weight entries remain fixed.
 */
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

/* Gather visible children, returning FALSE on table allocation failure and
 * TRUE for an empty group. Start at each basis, grow or shrink, distribute
 * nonnegative remaining slack by Justify, and size the cross axis by Align.
 * Call MUI_Layout for each child but ignore its result. Return TRUE even if
 * limits force content beyond available space.
 */
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

/* Dispatch the class-owned hook protocol using instance storage embedded
 * with the hook. MINMAX fills bounds and returns zero; LAYOUT returns the
 * layout helper result; other message types return MUILM_UNKNOWN. The
 * object argument is unused and no ownership is transferred.
 */
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
/* Install the instance hook with a superclass OM_SET call that bypasses
 * Flexgroup and sets Group_Forward FALSE for this call. This prevents
 * forwarding this replacement hook to child groups; it does not suppress
 * Notify or undo a previously forwarded caller hook. Ignore the superclass
 * result.
 */
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

/* Create the Group superclass and return NULL on failure. Initialize the
 * embedded HookEntry callback and read the first occurrence of each local
 * attribute with defaults. Claim the layout hook afterward. A supplied non-
 * NULL hook emits a diagnostic; no custom hook remains installed on this
 * object.
 */
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

/* Free the owned cached calculation table, then delegate disposal of the
 * Group and its members. Table entries only borrow child pointers; this
 * method does not separately dispose them. Return the superclass disposal
 * result.
 */
IPTR Flexgroup__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Flexgroup_DATA *data = INST_DATA(cl, obj);

    FreeVec(data->calc);

    return DoSuperMethodA(cl, obj, msg);
}

/* Store local values and remember only their last tag occurrences.
 * Temporarily hide those tags for the first superclass pass, restore them,
 * reclaim any caller-replaced hook, then notify in a separate nonforwarding
 * pass and request relayout when render info exists. Require writable tags.
 * Earlier duplicates and omitted NoNotify in the second pass remain
 * observable. Return only the first superclass result.
 */
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

/* Return the converted stored Direction, Justify, Align, or Gap value with
 * TRUE. These getters do not validate selector ranges or perform layout.
 * Forward unknown attribute queries to Group and return its result.
 */
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
