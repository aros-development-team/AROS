/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library — utility.library's tag item functions for m68k
    programs, on tag lists in the program's memory.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <string.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"

#define T(name) IPTR name(struct M68KEmuContext *ctx, void *cpu)

static BOOL item_ok(struct M68KEmuContext *ctx, ULONG ti)
{
    return ti && ti + 8 <= ctx->mem_size;
}

/* the next user item from *tp, following TAG_MORE and TAG_SKIP; 0 at the end */
static ULONG next_item(struct M68KEmuContext *ctx, ULONG *tp)
{
    ULONG ti = *tp;

    for (int guard = 0; guard < 65536 && item_ok(ctx, ti); guard++)
    {
        ULONG tag = m68k_read32(ctx, ti), data = m68k_read32(ctx, ti + 4);

        switch (tag)
        {
        case TAG_DONE:
            *tp = 0;
            return 0;
        case TAG_IGNORE:
            ti += 8;
            break;
        case TAG_MORE:
            ti = data;
            break;
        case TAG_SKIP:
            ti += 8 * (data + 1);
            break;
        default:
            *tp = ti + 8;
            return ti;
        }
    }
    *tp = 0;
    return 0;
}

/* for the thunks: the program's tag lists, item by item */
ULONG m68k_next_tag(struct M68KEmuContext *ctx, ULONG *state)
{
    return next_item(ctx, state);
}

static ULONG find_item(struct M68KEmuContext *ctx, ULONG tag, ULONG list)
{
    ULONG ti;

    while ((ti = next_item(ctx, &list)))
        if (m68k_read32(ctx, ti) == tag)
            return ti;
    return 0;
}

/* tag item arrays the program gets from AllocateTagItems: the size in front */
static ULONG alloc_items(struct M68KEmuContext *ctx, ULONG n)
{
    ULONG size = 8 + 8 * n, a;

    if (n > 0x100000 || !(a = M68KEmu_HeapAlloc(ctx, size, 0)))
        return 0;
    memset(ctx->mem + a, 0, size);
    m68k_write32(ctx, a, size);
    return a + 8;
}

/* -30: FindTagItem(D0=tagVal, A0=tagList) */
T(m68k_utility_FindTagItem)
{
    return find_item(ctx, M68KEmu_GetD(cpu, 0), M68KEmu_GetA(cpu, 0));
}

/* -36: GetTagData(D0=tagValue, D1=defaultVal, A0=tagList) */
T(m68k_utility_GetTagData)
{
    ULONG ti = find_item(ctx, M68KEmu_GetD(cpu, 0), M68KEmu_GetA(cpu, 0));
    return ti ? m68k_read32(ctx, ti + 4) : M68KEmu_GetD(cpu, 1);
}

/* -42: PackBoolTags(D0=initialFlags, A0=tagList, A1=boolMap) */
T(m68k_utility_PackBoolTags)
{
    ULONG flags = M68KEmu_GetD(cpu, 0), list = M68KEmu_GetA(cpu, 0), map = M68KEmu_GetA(cpu, 1), ti;

    while ((ti = next_item(ctx, &list)))
    {
        ULONG mi = find_item(ctx, m68k_read32(ctx, ti), map);
        if (!mi)
            continue;
        if (m68k_read32(ctx, ti + 4))
            flags |= m68k_read32(ctx, mi + 4);
        else
            flags &= ~m68k_read32(ctx, mi + 4);
    }
    return flags;
}

/* -48: NextTagItem(A0=tagListPtr) */
T(m68k_utility_NextTagItem)
{
    ULONG tpp = M68KEmu_GetA(cpu, 0), tp, ti;

    if (!tpp || tpp + 4 > ctx->mem_size)
        return 0;
    tp = m68k_read32(ctx, tpp);
    ti = next_item(ctx, &tp);
    m68k_write32(ctx, tpp, tp);
    return ti;
}

/* -54: FilterTagChanges(A0=changeList, A1=originalList, D0=apply) */
T(m68k_utility_FilterTagChanges)
{
    ULONG list = M68KEmu_GetA(cpu, 0), orig = M68KEmu_GetA(cpu, 1), apply = M68KEmu_GetD(cpu, 0), ti;

    while ((ti = next_item(ctx, &list)))
    {
        ULONG oi = find_item(ctx, m68k_read32(ctx, ti), orig);
        if (!oi)
            continue;
        if (m68k_read32(ctx, oi + 4) == m68k_read32(ctx, ti + 4))
            m68k_write32(ctx, ti, TAG_IGNORE);
        else if (apply)
            m68k_write32(ctx, oi + 4, m68k_read32(ctx, ti + 4));
    }
    return 0;
}

/* -60: MapTags(A0=tagList, A1=mapList, D0=mapType) */
T(m68k_utility_MapTags)
{
    ULONG list = M68KEmu_GetA(cpu, 0), map = M68KEmu_GetA(cpu, 1), type = M68KEmu_GetD(cpu, 0), ti;

    while ((ti = next_item(ctx, &list)))
    {
        ULONG mi = find_item(ctx, m68k_read32(ctx, ti), map);
        if (mi)
        {
            ULONG to = m68k_read32(ctx, mi + 4);
            m68k_write32(ctx, ti, to == TAG_DONE ? TAG_IGNORE : to);
        }
        else if (type == MAP_REMOVE_NOT_FOUND)
            m68k_write32(ctx, ti, TAG_IGNORE);
    }
    return 0;
}

/* -66: AllocateTagItems(D0=numTags) */
T(m68k_utility_AllocateTagItems)
{
    return alloc_items(ctx, M68KEmu_GetD(cpu, 0));
}

/* -72: CloneTagItems(A0=original) */
T(m68k_utility_CloneTagItems)
{
    ULONG list = M68KEmu_GetA(cpu, 0), n = 0, ti, clone, c;

    for (ULONG l = list; next_item(ctx, &l); )
        n++;
    if (!(clone = alloc_items(ctx, n + 1)))
        return 0;
    for (c = clone; (ti = next_item(ctx, &list)); c += 8)
    {
        m68k_write32(ctx, c, m68k_read32(ctx, ti));
        m68k_write32(ctx, c + 4, m68k_read32(ctx, ti + 4));
    }
    return clone;
}

/* -78: FreeTagItems(A0=tagList) */
T(m68k_utility_FreeTagItems)
{
    ULONG list = M68KEmu_GetA(cpu, 0);

    if (list >= 8 && list <= ctx->mem_size)
        M68KEmu_HeapFree(ctx, list - 8, m68k_read32(ctx, list - 8));
    return 0;
}

/* -84: RefreshTagItemClones(A0=clone, A1=original) */
T(m68k_utility_RefreshTagItemClones)
{
    ULONG clone = M68KEmu_GetA(cpu, 0), list = M68KEmu_GetA(cpu, 1), ti;

    if (!clone)
        return 0;
    while ((ti = next_item(ctx, &list)) && item_ok(ctx, clone))
    {
        m68k_write32(ctx, clone, m68k_read32(ctx, ti));
        m68k_write32(ctx, clone + 4, m68k_read32(ctx, ti + 4));
        clone += 8;
    }
    if (item_ok(ctx, clone))
        m68k_write32(ctx, clone, TAG_DONE);
    return 0;
}

static BOOL in_array(struct M68KEmuContext *ctx, ULONG tag, ULONG array)
{
    ULONG t;

    for (; array && array + 4 <= ctx->mem_size && (t = m68k_read32(ctx, array)) != TAG_DONE; array += 4)
        if (t == tag)
            return TRUE;
    return FALSE;
}

/* -90: TagInArray(D0=tag, A0=tagArray) */
T(m68k_utility_TagInArray)
{
    return in_array(ctx, M68KEmu_GetD(cpu, 0), M68KEmu_GetA(cpu, 0));
}

/* -96: FilterTagItems(A0=tagList, A1=filterArray, D0=logic) */
T(m68k_utility_FilterTagItems)
{
    ULONG list = M68KEmu_GetA(cpu, 0), filter = M68KEmu_GetA(cpu, 1), logic = M68KEmu_GetD(cpu, 0), ti, n = 0;

    while ((ti = next_item(ctx, &list)))
    {
        BOOL in = in_array(ctx, m68k_read32(ctx, ti), filter);
        if ((logic == TAGFILTER_AND && !in) || (logic == TAGFILTER_NOT && in))
            m68k_write32(ctx, ti, TAG_IGNORE);
        else
            n++;
    }
    return n;
}

/* -102: CallHookPkt(A0=hook, A2=object, A1=paramPacket): a hook in the
   program's memory runs as 68k code, entered as AmigaOS enters it */
T(m68k_utility_CallHookPkt)
{
    ULONG hook = M68KEmu_GetA(cpu, 0), entry;

    if (!hook || hook + 20 > ctx->mem_size || !(entry = m68k_read32(ctx, hook + 8)) || entry >= ctx->mem_size)
        return 0;
    ctx->tail_pc = entry;
    ctx->tail_d0 = 0;
    ctx->tail_a0 = hook;
    ctx->tail_a6 = M68KEmu_GetA(cpu, 6);
    return 0;
}

/* -150 SDivMod32, -156 UDivMod32 (D0=dividend, D1=divisor): the quotient
   in D0 and the remainder in D1 */
T(m68k_utility_SDivMod32)
{
    LONG a = (LONG)M68KEmu_GetD(cpu, 0), b = (LONG)M68KEmu_GetD(cpu, 1);
    (void)ctx;
    if (!b)
        return 0;
    M68KEmu_SetD(cpu, 1, (ULONG)(a % b));
    return (ULONG)(a / b);
}

T(m68k_utility_UDivMod32)
{
    ULONG a = M68KEmu_GetD(cpu, 0), b = M68KEmu_GetD(cpu, 1);
    (void)ctx;
    if (!b)
        return 0;
    M68KEmu_SetD(cpu, 1, a % b);
    return a / b;
}

/* -198 SMult64, -204 UMult64 (D0, D1): the 64-bit product, high half in D0 */
T(m68k_utility_SMult64)
{
    QUAD p = (QUAD)(LONG)M68KEmu_GetD(cpu, 0) * (QUAD)(LONG)M68KEmu_GetD(cpu, 1);
    (void)ctx;
    M68KEmu_SetD(cpu, 1, (ULONG)p);
    return (ULONG)((UQUAD)p >> 32);
}

T(m68k_utility_UMult64)
{
    UQUAD p = (UQUAD)M68KEmu_GetD(cpu, 0) * (UQUAD)M68KEmu_GetD(cpu, 1);
    (void)ctx;
    M68KEmu_SetD(cpu, 1, (ULONG)p);
    return (ULONG)(p >> 32);
}

#undef T
