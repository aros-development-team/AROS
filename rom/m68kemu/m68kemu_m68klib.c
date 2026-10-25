/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)
*/
/* m68kemu_m68klib.c — Load m68k .library files into containment */

#include <stdio.h>
#include <aros/debug.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#include <exec/nodes.h>
#include <exec/libraries.h>
#include "m68kemu_intern.h"
#include "m68kemu_offsets.h"

/* RTC_MATCHWORD, RTF_AUTOINIT now in m68kemu_offsets.h (as M68K_RTC_MATCHWORD, M68K_RTF_AUTOINIT) */
#define NT_LIBRARY    9

/*
 * Load an m68k .library file from LIBS: into containment.
 * Scans for the Resident tag, reads the function table,
 * and builds a proper m68k jump table.
 *
 * Returns the m68k library base address, or 0 on failure.
 */
/* exec InitStruct() on m68k memory: command byte ddssnnnn - dd 00 copy n+1
   items here, 01 repeat one item n+1 times, 10 byte offset follows, 11
   24-bit offset follows; ss 00 LONG, 01 WORD, 10 BYTE; WORD/LONG data is
   word aligned; a zero command ends the table. */
static void m68k_init_struct(struct M68KEmuContext *ctx, ULONG tab, ULONG mem)
{
    ULONG dst = mem, p = tab;
    int guard = 0;

    while (guard++ < 4096)
    {
        UBYTE cmd = ctx->mem[p++];
        UBYTE kind = cmd >> 6, size = (cmd >> 4) & 3;
        ULONG count = (cmd & 15) + 1, i, sz;

        if (!cmd) break;
        if (size == 3) break;
        sz = size == 0 ? 4 : size == 1 ? 2 : 1;
        if (kind == 2)
            dst = mem + ctx->mem[p++];
        else if (kind == 3)
        {
            dst = mem + ((ULONG)ctx->mem[p] << 16 | (ULONG)ctx->mem[p + 1] << 8 | ctx->mem[p + 2]);
            p += 3;
        }
        if (sz > 1 && (p & 1)) p++;
        for (i = 0; i < count; i++)
        {
            ULONG src = (kind == 1) ? p : p + i * sz;
            if (sz == 4) m68k_write32(ctx, dst, m68k_read32(ctx, src));
            else if (sz == 2) m68k_write16(ctx, dst, m68k_read16(ctx, src));
            else ctx->mem[dst] = ctx->mem[src];
            dst += sz;
        }
        p += (kind == 1) ? sz : count * sz;
        if (p & 1) p++;
    }
}

ULONG M68KEmu_LoadM68KLibrary(struct M68KEmuContext *ctx, const char *name)
{
    char path[256];
    BPTR fh;
    LONG fileSize;
    UBYTE *fileData;
    ULONG entry, base = 0;
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldwin = me->pr_WindowPtr;

    /* A probe for an optional library must not raise a volume requester */
    me->pr_WindowPtr = (APTR)-1;

    /* m68k libraries without a native replacement live in LIBS68K:,
       SYS:Libs68k/ when that assign is not set */
    {
        static const char *const dirs[] = { "LIBS68K:", "SYS:Libs68k/", "LIBS:m68k/", "LIBS:" };
        ULONG d;

        fh = BNULL;
        for (d = 0; d < sizeof(dirs) / sizeof(dirs[0]) && !fh; d++)
        {
            ULONG magic = 0;

            snprintf(path, sizeof(path), "%s%s", dirs[d], name);
            fh = Open(path, MODE_OLDFILE);
            /* only a hunk file: LIBS: also holds the native libraries */
            if (fh && (Read(fh, &magic, 4) != 4 || AROS_BE2LONG(magic) != 0x3F3))
            {
                Close(fh);
                fh = BNULL;
            }
        }
    }

    me->pr_WindowPtr = oldwin;

    if (!fh) return 0;

    Seek(fh, 0, OFFSET_END);
    fileSize = Seek(fh, 0, OFFSET_BEGINNING);
    if (fileSize <= 0) { Close(fh); return 0; }

    fileData = (UBYTE *)AllocMem(fileSize, MEMF_ANY);
    if (!fileData) { Close(fh); return 0; }

    if (Read(fh, fileData, fileSize) != fileSize)
    { FreeMem(fileData, fileSize); Close(fh); return 0; }
    Close(fh);

    /* Load hunks into containment */
    entry = M68KEmu_LoadHunksFromMemory(ctx, fileData, fileSize);
    FreeMem(fileData, fileSize);

    if (!entry)
    {
        m68k_log(ctx, "68k library %s is not a usable hunk file", name);
        return 0;
    }

    /* Scan containment for M68K_RTC_MATCHWORD (0x4AFC) */
    ULONG rt_addr = 0;
    /* Scan every hunk of the library: the Resident can live in its data hunk */
    for (ULONG addr = ctx->load_lo & ~1; addr + M68K_RT_MATCHTAG + 4 <= ctx->load_hi; addr += 2)
    {
        if (m68k_read16(ctx, addr) == M68K_RTC_MATCHWORD)
        {
            /* Verify rt_MatchTag points back to itself */
            ULONG mt = m68k_read32(ctx, addr + M68K_RT_MATCHTAG);
            if (mt == addr)
            {
                rt_addr = addr;
                break;
            }
        }
    }

    if (!rt_addr)
    {
        m68k_log(ctx, "68k library %s has no Resident tag", name);
        return 0;
    }

    /* Read Resident struct fields */
    UBYTE rt_Flags   = ctx->mem[rt_addr + M68K_RT_FLAGS];
    UBYTE rt_Type    = ctx->mem[rt_addr + M68K_RT_TYPE];
    ULONG rt_Init    = m68k_read32(ctx, rt_addr + M68K_RT_INIT);

    if (ctx->trace_calls) bug("[m68kemu] m68klib: %s resident at 0x%lx flags=0x%x type=%d init=0x%lx\n",
        name, (unsigned long)rt_addr, rt_Flags, rt_Type, (unsigned long)rt_Init);

    if (!(rt_Flags & M68K_RTF_AUTOINIT))
    {
        /* Non-AUTOINIT: rt_Init is a function pointer.
           For now, we'd need to call it. Skip for now. */
        bug("[m68kemu] m68klib: %s is not M68K_RTF_AUTOINIT, not supported yet\n", name);
        return 0;
    }

    /* M68K_RTF_AUTOINIT: rt_Init points to init table:
       ULONG dataSize, APTR funcTable, APTR dataInit, APTR initFunc */
    ULONG dataSize  = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_DATASIZE);
    ULONG funcTable = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_FUNCTABLE);
    ULONG dataInit  = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_DATAINIT);
    ULONG initFunc  = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_INITFUNC);

    if (ctx->trace_calls) bug("[m68kemu] m68klib: dataSize=%lu funcTable=0x%lx dataInit=0x%lx initFunc=0x%lx\n",
        (unsigned long)dataSize, (unsigned long)funcTable, (unsigned long)dataInit, (unsigned long)initFunc);

    if (!funcTable) return 0;

    /* Function table: absolute LONGs ending in -1, or - when it starts with
       the WORD -1 - WORD offsets from the table ending in -1 */
    BOOL rel = m68k_read16(ctx, funcTable) == 0xFFFF;
    UWORD numFuncs = 0;
    if (rel)
        while (m68k_read16(ctx, funcTable + 2 + numFuncs * 2) != 0xFFFF && numFuncs < 1000)
            numFuncs++;
    else
        while (m68k_read32(ctx, funcTable + numFuncs * 4) != M68K_FUNCTABLE_END && numFuncs < 1000)
            numFuncs++;

    if (numFuncs == 0) return 0;

    if (ctx->trace_calls) bug("[m68kemu] m68klib: %s has %d functions\n", name, numFuncs);

    /* Allocate library base with jump table.
       Jump table: numFuncs * 6 bytes (JMP abs.l = 4E F9 xx xx xx xx)
       at negative offsets from base.
       Base itself: dataSize bytes for the library data. */
    if (dataSize < M68K_SIZEOF_LIBRARY) dataSize = M68K_SIZEOF_LIBRARY;  /* minimum Library struct */
    ULONG jt_size = (ULONG)numFuncs * M68K_JT_SLOT_SIZE;
    ULONG total = jt_size + dataSize;
    ULONG region = M68KEmu_HeapAlloc(ctx, total, 0);
    if (!region) return 0;

    base = region + jt_size;  /* base is after the jump table */

    /* Build jump table: JMP abs.l for each function */
    for (UWORD i = 0; i < numFuncs; i++)
    {
        ULONG func_addr = rel ? funcTable + (LONG)(WORD)m68k_read16(ctx, funcTable + 2 + i * 2)
                              : m68k_read32(ctx, funcTable + i * 4);

        ULONG slot = base - ((ULONG)(i + 1) * M68K_JT_SLOT_SIZE);
        m68k_write16(ctx, slot,     M68K_OP_JMP_ABS_L);
        m68k_write32(ctx, slot + 2, func_addr);
    }

    memset(ctx->mem + base, 0, dataSize);

    /* Library node: ln_Type, ln_Name, lib_Flags, lib_NegSize, lib_PosSize, lib_Version, lib_IdString */
    UBYTE rt_Version = ctx->mem[rt_addr + M68K_RT_VERSION];
    ULONG rt_Name = m68k_read32(ctx, rt_addr + M68K_RT_NAME);
    ctx->mem[base + 8] = NT_LIBRARY;
    m68k_write32(ctx, base + M68K_LIB_LNNAME, rt_Name);
    ctx->mem[base + 14] = LIBF_SUMUSED | LIBF_CHANGED;
    m68k_write16(ctx, base + 16, (UWORD)jt_size);
    m68k_write16(ctx, base + 18, (UWORD)dataSize);
    m68k_write16(ctx, base + M68K_LIB_VERSION, rt_Version);
    m68k_write32(ctx, base + 24, m68k_read32(ctx, rt_addr + 18));   /* lib_IdString = rt_IdString */

    if (dataInit)
        m68k_init_struct(ctx, dataInit, base);

    /* The library's init function stores the bases its functions use; it
       runs as 68k code when the program's OpenLibrary returns (tail call:
       D0 = base, A0 = seglist, A6 = SysBase), and its result is the
       OpenLibrary result. */
    if (initFunc)
    {
        ctx->tail_pc = initFunc;
        ctx->tail_d0 = base;
        ctx->tail_a0 = 0;
        ctx->tail_a6 = ctx->m68k_sysbase;
    }

    if (ctx->trace_calls) bug("[m68kemu] m68klib: %s loaded at base=0x%lx (%d funcs)\n",
        name, (unsigned long)base, numFuncs);

    return base;
}
