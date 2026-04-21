/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
*/
/* m68kemu_m68klib.c — Load m68k .library files into containment */

#include <aros/debug.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

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
ULONG M68KEmu_LoadM68KLibrary(struct M68KEmuContext *ctx, const char *name)
{
    char path[256];
    BPTR fh;
    LONG fileSize;
    UBYTE *fileData;
    ULONG entry, base = 0;

    /* Try LIBS68K:name first (dedicated assign for m68k libraries) */
    snprintf(path, sizeof(path), "LIBS68K:%s", name);
    fh = Open(path, MODE_OLDFILE);

    /* Try LIBS:m68k/name */
    if (!fh)
    {
        snprintf(path, sizeof(path), "LIBS:m68k/%s", name);
        fh = Open(path, MODE_OLDFILE);
    }

    /* Fall back to LIBS:name */
    if (!fh)
    {
        snprintf(path, sizeof(path), "LIBS:%s", name);
        fh = Open(path, MODE_OLDFILE);
    }

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
        bug("[m68kemu] m68klib: failed to load hunks for %s\n", name);
        return 0;
    }

    /* Scan containment for M68K_RTC_MATCHWORD (0x4AFC) */
    ULONG rt_addr = 0;
    /* Hunks loaded at high addresses — scan from entry through loaded region */
    for (ULONG addr = entry & ~1; addr < ctx->mem_size; addr += 2)
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
        bug("[m68kemu] m68klib: no resident tag in %s\n", name);
        return 0;
    }

    /* Read Resident struct fields */
    UBYTE rt_Flags   = ctx->mem[rt_addr + M68K_RT_FLAGS];
    UBYTE rt_Type    = ctx->mem[rt_addr + M68K_RT_TYPE];
    ULONG rt_Init    = m68k_read32(ctx, rt_addr + M68K_RT_INIT);

    bug("[m68kemu] m68klib: %s resident at 0x%lx flags=0x%x type=%d init=0x%lx\n",
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

    bug("[m68kemu] m68klib: dataSize=%lu funcTable=0x%lx initFunc=0x%lx\n",
        (unsigned long)dataSize, (unsigned long)funcTable, (unsigned long)initFunc);

    if (!funcTable) return 0;

    /* Count functions in the function table (terminated by -1 / 0xFFFFFFFF) */
    UWORD numFuncs = 0;
    ULONG ft = funcTable;
    while (m68k_read32(ctx, ft) != M68K_FUNCTABLE_END && numFuncs < 1000)
    {
        numFuncs++;
        ft += 4;  /* relative or absolute function pointers */
    }

    if (numFuncs == 0) return 0;

    bug("[m68kemu] m68klib: %s has %d functions\n", name, numFuncs);

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
        ULONG func_addr = m68k_read32(ctx, funcTable + i * 4);

        /* Check if relative (high bit set = relative to funcTable) */
        if (func_addr >= 0x80000000u)
        {
            /* Treat as relative offset from funcTable */
            func_addr = funcTable + (LONG)func_addr;
        }

        ULONG slot = base - ((ULONG)(i + 1) * M68K_JT_SLOT_SIZE);
        m68k_write16(ctx, slot,     M68K_OP_JMP_ABS_L);
        m68k_write32(ctx, slot + 2, func_addr);
    }

    /* Init minimal Library node at base */
    /* lib_Version at offset 20, lib_Revision at 22 */
    UBYTE rt_Version = ctx->mem[rt_addr + M68K_RT_VERSION];
    m68k_write16(ctx, base + M68K_LIB_VERSION, rt_Version);

    /* Store the library name pointer */
    ULONG rt_Name = m68k_read32(ctx, rt_addr + M68K_RT_NAME);
    m68k_write32(ctx, base + M68K_LIB_LNNAME, rt_Name);  /* ln_Name */

    bug("[m68kemu] m68klib: %s loaded at base=0x%lx (%d funcs)\n",
        name, (unsigned long)base, numFuncs);

    return base;
}
