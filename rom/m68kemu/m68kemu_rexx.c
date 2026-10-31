/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library — rexxsyslib.library for m68k programs. Argstrings
    and RexxMsgs are built in the program's memory in their 68k layout.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <stdio.h>
#include <string.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"

/* struct RexxArg: the argstring points at ra_Buff */
#define RA_SIZE     (-8)
#define RA_LENGTH   (-4)
#define RA_FLAGS    (-2)
#define RA_HASH     (-1)

/* struct RexxMsg */
#define RM_NAME      10
#define RM_REPLYPORT 14
#define RM_LENGTH    18
#define RM_LIBBASE   24
#define RM_ARGS      40
#define RM_COMMADDR  108
#define RM_FILEEXT   112
#define RM_SIZE      128

static ULONG m68k_strlen(struct M68KEmuContext *ctx, ULONG s)
{
    ULONG n = 0;
    while (s < ctx->mem_size && n < ctx->mem_size - s && ctx->mem[s + n])
        n++;
    return n;
}

static ULONG rx_alloc(struct M68KEmuContext *ctx, ULONG size)
{
    ULONG a = M68KEmu_HeapAlloc(ctx, size, MEMF_PUBLIC);
    if (a)
        memset(ctx->mem + a, 0, size);
    return a;
}

static ULONG create_argstring(struct M68KEmuContext *ctx, const UBYTE *s, ULONG len)
{
    ULONG size = 8 + len + 1, ra, hash = 0;

    if (len > 0xffff || !(ra = rx_alloc(ctx, size)))
        return 0;
    for (ULONG i = 0; i < len; i++)
        hash += s[i];
    m68k_write32(ctx, ra, size);
    m68k_write16(ctx, ra + 4, len);
    ctx->mem[ra + 6] = (1 << 1) | (1 << 2) | (1 << 6);   /* NSF_ALPHA | NSF_EXT */
    ctx->mem[ra + 7] = hash & 0xff;
    memcpy(ctx->mem + ra + 8, s, len);
    return ra + 8;
}

static void delete_argstring(struct M68KEmuContext *ctx, ULONG as)
{
    if (m68k_in_mem(ctx, as + RA_SIZE, 8))
        M68KEmu_HeapFree(ctx, as + RA_SIZE, m68k_read32(ctx, as + RA_SIZE));
}

/* the ln_Name every RexxMsg carries */
static ULONG rexx_id(struct M68KEmuContext *ctx)
{
    if (!ctx->rexx_id && (ctx->rexx_id = rx_alloc(ctx, 5)))
        memcpy(ctx->mem + ctx->rexx_id, "REXX", 5);
    return ctx->rexx_id;
}

#define T(name) static IPTR name(struct M68KEmuContext *ctx, void *cpu)

/* -126: CreateArgstring(A0=string, D0=length) */
T(rx_CreateArgstring)
{
    ULONG s = M68KEmu_GetA(cpu, 0), len = M68KEmu_GetD(cpu, 0);

    if (len && !m68k_in_mem(ctx, s, len))
        return 0;
    return create_argstring(ctx, ctx->mem + s, len);
}

/* -132: DeleteArgstring(A0=argstring) */
T(rx_DeleteArgstring)
{
    delete_argstring(ctx, M68KEmu_GetA(cpu, 0));
    return 0;
}

/* -138: LengthArgstring(A0=argstring) */
T(rx_LengthArgstring)
{
    ULONG as = M68KEmu_GetA(cpu, 0);
    return m68k_in_mem(ctx, as + RA_LENGTH, 2) ? m68k_read16(ctx, as + RA_LENGTH) : 0;
}

/* -144: CreateRexxMsg(A0=port, A1=extension, D0=host) */
T(rx_CreateRexxMsg)
{
    ULONG rm = rx_alloc(ctx, RM_SIZE);

    if (!rm)
        return 0;
    ctx->mem[rm + 8] = NT_MESSAGE;
    m68k_write32(ctx, rm + RM_NAME, rexx_id(ctx));
    m68k_write32(ctx, rm + RM_REPLYPORT, M68KEmu_GetA(cpu, 0));
    m68k_write16(ctx, rm + RM_LENGTH, RM_SIZE);
    m68k_write32(ctx, rm + RM_LIBBASE, M68KEmu_FindLibBase(ctx, "rexxsyslib.library"));
    m68k_write32(ctx, rm + RM_FILEEXT, M68KEmu_GetA(cpu, 1));
    m68k_write32(ctx, rm + RM_COMMADDR, M68KEmu_GetD(cpu, 0));
    return rm;
}

/* -150: DeleteRexxMsg(A0=packet) */
T(rx_DeleteRexxMsg)
{
    ULONG rm = M68KEmu_GetA(cpu, 0);

    if (m68k_in_mem(ctx, rm, RM_SIZE))
        M68KEmu_HeapFree(ctx, rm, m68k_read16(ctx, rm + RM_LENGTH));
    return 0;
}

/* -156: ClearRexxMsg(A0=msgptr, D0=count) */
T(rx_ClearRexxMsg)
{
    ULONG rm = M68KEmu_GetA(cpu, 0), count = M68KEmu_GetD(cpu, 0);

    if (!m68k_in_mem(ctx, rm, RM_SIZE) || count > 16)
        return 0;
    for (ULONG i = 0; i < count; i++)
    {
        ULONG as = m68k_read32(ctx, rm + RM_ARGS + 4 * i);
        if (as)
        {
            delete_argstring(ctx, as);
            m68k_write32(ctx, rm + RM_ARGS + 4 * i, 0);
        }
    }
    return 0;
}

/* -162: FillRexxMsg(A0=msgptr, D0=count, D1=mask) */
T(rx_FillRexxMsg)
{
    ULONG rm = M68KEmu_GetA(cpu, 0), count = M68KEmu_GetD(cpu, 0), mask = M68KEmu_GetD(cpu, 1);
    ULONG args[16];

    if (!m68k_in_mem(ctx, rm, RM_SIZE) || count > 16)
        return FALSE;
    for (ULONG i = 0; i < count; i++)
    {
        ULONG v = m68k_read32(ctx, rm + RM_ARGS + 4 * i);
        char number[16];

        if (mask & (1 << i))
        {
            snprintf(number, sizeof(number), "%ld", (long)(LONG)v);
            args[i] = create_argstring(ctx, (const UBYTE *)number, strlen(number));
        }
        else if (!v)
        {
            args[i] = 0;
            continue;
        }
        else
            args[i] = m68k_in_mem(ctx, v, 1) ? create_argstring(ctx, ctx->mem + v, m68k_strlen(ctx, v)) : 0;
        if (!args[i])
        {
            for (ULONG j = 0; j < i; j++)
                if (args[j])
                    delete_argstring(ctx, args[j]);
            return FALSE;
        }
    }
    for (ULONG i = 0; i < count; i++)
        m68k_write32(ctx, rm + RM_ARGS + 4 * i, args[i]);
    return TRUE;
}

/* -168: IsRexxMsg(A0=msgptr) */
T(rx_IsRexxMsg)
{
    ULONG rm = M68KEmu_GetA(cpu, 0), name;

    if (!m68k_in_mem(ctx, rm, RM_SIZE))
        return FALSE;
    name = m68k_read32(ctx, rm + RM_NAME);
    return name && (name == ctx->rexx_id || (m68k_in_mem(ctx, name, 5) && !memcmp(ctx->mem + name, "REXX", 5)));
}

/* -450/-456: LockRexxBase/UnlockRexxBase(D0=resource): one program, nothing shared to lock */
T(rx_Nop)
{
    (void)ctx; (void)cpu;
    return 0;
}

#undef T

static const struct M68KThunkEntry rexx_tab[] = {
    { 126, rx_CreateArgstring },
    { 132, rx_DeleteArgstring },
    { 138, rx_LengthArgstring },
    { 144, rx_CreateRexxMsg },
    { 150, rx_DeleteRexxMsg },
    { 156, rx_ClearRexxMsg },
    { 162, rx_FillRexxMsg },
    { 168, rx_IsRexxMsg },
    { 450, rx_Nop },             /* LockRexxBase */
    { 456, rx_Nop },             /* UnlockRexxBase */
};

const struct M68KLibThunkSet m68kemu_rexx_lib = { "rexxsyslib.library", rexx_tab, sizeof(rexx_tab) / sizeof(rexx_tab[0]) };
