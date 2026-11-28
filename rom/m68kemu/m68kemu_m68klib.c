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
   word aligned; a zero command ends the table.
   The table comes from the file and is not trusted. FALSE, with the
   structure partly written, when a command or one of its items lies
   outside containment, when the size code is 3, or when an item would be
   written outside the size bytes of the structure at mem. */
static BOOL m68k_init_struct(struct M68KEmuContext *ctx, ULONG tab, ULONG mem, ULONG size)
{
    ULONG off = 0, p = tab;     /* off: where the next item goes, from mem */
    int guard = 0;

    if (!m68k_in_mem(ctx, mem, size))
        return FALSE;

    while (guard++ < 4096)
    {
        UBYTE cmd, kind, esize;
        ULONG count, i, sz, bytes;

        if (!m68k_in_mem(ctx, p, 1))
            return FALSE;
        cmd = ctx->mem[p++];
        kind = cmd >> 6;
        esize = (cmd >> 4) & 3;
        count = (cmd & 15) + 1;

        if (!cmd) break;
        if (esize == 3) return FALSE;
        sz = esize == 0 ? 4 : esize == 1 ? 2 : 1;
        if (kind == 2)
        {
            if (!m68k_in_mem(ctx, p, 1))
                return FALSE;
            off = ctx->mem[p++];
        }
        else if (kind == 3)
        {
            if (!m68k_in_mem(ctx, p, 3))
                return FALSE;
            off = (ULONG)ctx->mem[p] << 16 | (ULONG)ctx->mem[p + 1] << 8 | ctx->mem[p + 2];
            p += 3;
        }
        if (sz > 1 && (p & 1)) p++;

        /* the items of this command, and the place they go to */
        bytes = count * sz;
        if (!m68k_in_mem(ctx, p, kind == 1 ? sz : bytes))
            return FALSE;
        if (off > size || bytes > size - off)
            return FALSE;
        for (i = 0; i < count; i++)
        {
            ULONG src = (kind == 1) ? p : p + i * sz;
            ULONG dst = mem + off;
            if (sz == 4) m68k_write32(ctx, dst, m68k_read32(ctx, src));
            else if (sz == 2) m68k_write16(ctx, dst, m68k_read16(ctx, src));
            else ctx->mem[dst] = ctx->mem[src];
            off += sz;
        }
        p += (kind == 1) ? sz : bytes;
        if (p & 1) p++;
    }
    return TRUE;
}

/* The vectors of a function table, as exec's MakeFunctions() reads them:
   LONG addresses ending in -1, or, with rel, WORD displacements ending in
   the WORD -1. The table is not trusted: every entry that counts has to lie
   in containment. The number of vectors before the end; -1 when the table
   runs out of containment first; M68K_MAX_VECTORS + 1 when there is no end
   within M68K_MAX_VECTORS vectors. The loader cuts such a table at
   M68K_MAX_VECTORS, as it always has; MakeLibrary() and MakeFunctions()
   refuse it rather than build a library with vectors missing. */
static LONG m68k_count_vectors(struct M68KEmuContext *ctx, ULONG array, BOOL rel)
{
    for (LONG n = 0; n <= M68K_MAX_VECTORS; n++)
    {
        ULONG at = rel ? array + n * 2 : array + n * 4;

        if (!m68k_in_mem(ctx, at, rel ? 2 : 4))
            return n < M68K_MAX_VECTORS ? -1 : M68K_MAX_VECTORS + 1;
        if (rel ? m68k_read16(ctx, at) == 0xFFFF : m68k_read32(ctx, at) == M68K_FUNCTABLE_END)
            return n;
    }
    return M68K_MAX_VECTORS + 1;
}

/* The jump table below target, as exec's MakeFunctions() builds it: vector
   n, at target - 6n, is a JMP to the n-th address of the table, or, with
   rel, to disp plus the n-th displacement. A zero entry gets exec's empty
   vector. The table was counted by m68k_count_vectors(), and the caller has
   checked that the count slots below target lie in containment. Only 68k
   addresses go into the slots. */
static void m68k_put_vectors(struct M68KEmuContext *ctx, ULONG target, ULONG array, BOOL rel, ULONG disp, ULONG count)
{
    for (ULONG i = 0; i < count; i++)
    {
        ULONG slot = target - (i + 1) * M68K_JT_SLOT_SIZE;
        ULONG func;

        if (rel)
        {
            WORD d = (WORD)m68k_read16(ctx, array + i * 2);
            func = d ? disp + (LONG)d : 0;
        }
        else
            func = m68k_read32(ctx, array + i * 4);
        if (!func)
            func = M68K_EMPTY_VECTOR | ((i + 1) << 8);
        m68k_write16(ctx, slot, M68K_OP_JMP_ABS_L);
        m68k_write32(ctx, slot + 2, func);
    }
}

/* Nothing of an attempt that did not get as far as a usable library is
   kept. What goes back is exactly what that attempt took from the heap: the
   base with its jump table, when it was allocated already, and the hunks of
   the image, through the chain the loader gave them. Nothing is derived
   from a span of addresses, and nothing of the running program is touched. */
static ULONG m68k_library_abandon(struct M68KEmuContext *ctx, ULONG entry, ULONG region, ULONG total)
{
    if (region)
        M68KEmu_HeapFree(ctx, region, total);
    M68KEmu_FreeImage(ctx, entry);
    return 0;
}

ULONG M68KEmu_LoadM68KLibrary(struct M68KEmuContext *ctx, const char *name, ULONG version)
{
    char path[256];
    BPTR fh;
    LONG fileSize;
    UBYTE *fileData;
    ULONG entry, base = 0, region = 0, total = 0;
    struct M68KTaskState *ts;
    /* The internal API keeps its full-width version parameter. */
    if (version > 0xFFFFu) return 0;

    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldwin = me->pr_WindowPtr;
    /* Looking for the file and reading it are the loader's own business:
       the caller's IoErr() stays what it was, found or not, as when LDDemon
       loads a native library in its own process (rom/lddemon). The code of
       the library may still set it, as the caller's own code. */
    LONG oldErr = IoErr();

    /* A probe for an optional library must not raise a volume requester */
    me->pr_WindowPtr = (APTR)-1;

    /* m68k libraries without a native replacement live in LIBS68K:,
       SYS:Libs68k/ when that assign is not set */
    {
        static const char *const dirs[] = { "LIBS68K:", "SYS:Libs68k/", "LIBS:m68k/", "LIBS:" };
        ULONG d;

        BOOL is_path = strchr(name, ':') || strchr(name, '/');

        fh = BNULL;
        for (d = is_path ? 0 : 1; d <= sizeof(dirs) / sizeof(dirs[0]) && !fh; d++)
        {
            ULONG magic = 0;

            if (d == 0)
                snprintf(path, sizeof(path), "%s", name);
            else
                snprintf(path, sizeof(path), "%s%s", dirs[d - 1], name);
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

    if (!fh) { SetIoErr(oldErr); return 0; }

    Seek(fh, 0, OFFSET_END);
    fileSize = Seek(fh, 0, OFFSET_BEGINNING);
    if (fileSize <= 0) { Close(fh); SetIoErr(oldErr); return 0; }

    fileData = (UBYTE *)AllocMem(fileSize, MEMF_ANY);
    if (!fileData) { Close(fh); SetIoErr(oldErr); return 0; }

    if (Read(fh, fileData, fileSize) != fileSize)
    { FreeMem(fileData, fileSize); Close(fh); SetIoErr(oldErr); return 0; }
    Close(fh);
    SetIoErr(oldErr);

    /* Load hunks into containment. The loader describes the image it places
       in the fields that describe the running program. They are the
       program's again as soon as it returns, on every path out of here; the
       image of the library is known from here on by its entry point and by
       the chain of its hunks alone. */
    ULONG own_entry = ctx->entry_point, own_seg = ctx->seg_start;
    ULONG own_lo = ctx->load_lo, own_hi = ctx->load_hi;

    entry = M68KEmu_LoadHunksFromMemory(ctx, fileData, fileSize);
    FreeMem(fileData, fileSize);
    ctx->entry_point = own_entry;
    ctx->seg_start   = own_seg;
    ctx->load_lo     = own_lo;
    ctx->load_hi     = own_hi;

    if (!entry)
    {
        m68k_log(ctx, "68k library %s is not a usable hunk file", name);
        return 0;
    }

    /* Scan the hunks of the library for M68K_RTC_MATCHWORD (0x4AFC), in
       their order: the Resident can live in its data hunk. The hunks are
       taken from the image itself, as M68KEmu_LoadHunksFromMemory() chained
       them (at most 100), not from the span between the lowest and the
       highest one, which also holds whatever else lies between them. Only
       a Resident that is whole inside one hunk counts: its fields are read
       below. This is how LDInit() of rom/lddemon looks for the Resident of
       a native library: segment by segment, the first one that fits. */
    ULONG rt_addr = 0;
    {
        ULONG data = entry, size = 0, next = 0, hunks = 0;

        while (data && !rt_addr && hunks++ < 100 && M68KEmu_ImageHunk(ctx, data, &size, &next))
        {
            for (ULONG addr = (data + 1) & ~1u;
                 size >= M68K_SIZEOF_RESIDENT && addr - data <= size - M68K_SIZEOF_RESIDENT;
                 addr += 2)
            {
                /* Verify rt_MatchTag points back to itself */
                if (m68k_read16(ctx, addr) == M68K_RTC_MATCHWORD &&
                    m68k_read32(ctx, addr + M68K_RT_MATCHTAG) == addr)
                {
                    rt_addr = addr;
                    break;
                }
            }
            data = next;
        }
    }

    if (!rt_addr)
    {
        m68k_log(ctx, "68k library %s has no Resident tag", name);
        return m68k_library_abandon(ctx, entry, 0, 0);
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
        return m68k_library_abandon(ctx, entry, 0, 0);
    }

    /* M68K_RTF_AUTOINIT: rt_Init points to init table:
       ULONG dataSize, APTR funcTable, APTR dataInit, APTR initFunc.
       Its four longwords have to lie in containment. */
    if (!m68k_in_mem(ctx, rt_Init, 16))
    {
        m68k_log(ctx, "68k library %s: its init table is outside memory", name);
        return m68k_library_abandon(ctx, entry, 0, 0);
    }
    ULONG dataSize  = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_DATASIZE);
    ULONG funcTable = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_FUNCTABLE);
    ULONG dataInit  = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_DATAINIT);
    ULONG initFunc  = m68k_read32(ctx, rt_Init + M68K_AUTOINIT_INITFUNC);

    if (ctx->trace_calls) bug("[m68kemu] m68klib: dataSize=%lu funcTable=0x%lx dataInit=0x%lx initFunc=0x%lx\n",
        (unsigned long)dataSize, (unsigned long)funcTable, (unsigned long)dataInit, (unsigned long)initFunc);

    if (!funcTable) return m68k_library_abandon(ctx, entry, 0, 0);

    /* Function table: absolute LONGs ending in -1, or - when it starts with
       the WORD -1 - WORD offsets from the table ending in -1. Every entry
       that counts has to lie in containment: a table that runs out of it
       before its end is not one. */
    if (!m68k_in_mem(ctx, funcTable, 2)) return m68k_library_abandon(ctx, entry, 0, 0);
    BOOL rel = m68k_read16(ctx, funcTable) == 0xFFFF;
    ULONG vectors = rel ? funcTable + 2 : funcTable;
    LONG counted = m68k_count_vectors(ctx, vectors, rel);
    if (counted < 0)
    {
        m68k_log(ctx, "68k library %s: its function table runs out of memory", name);
        return m68k_library_abandon(ctx, entry, 0, 0);
    }
    UWORD numFuncs = (UWORD)(counted > M68K_MAX_VECTORS ? M68K_MAX_VECTORS : counted);

    if (numFuncs == 0) return m68k_library_abandon(ctx, entry, 0, 0);

    if (ctx->trace_calls) bug("[m68kemu] m68klib: %s has %d functions\n", name, numFuncs);

    /* Allocate library base with jump table.
       Jump table: numFuncs * 6 bytes (JMP abs.l = 4E F9 xx xx xx xx)
       at negative offsets from base.
       Base itself: dataSize bytes for the library data. */
    if (dataSize < M68K_SIZEOF_LIBRARY) dataSize = M68K_SIZEOF_LIBRARY;  /* minimum Library struct */
    ULONG jt_size = (ULONG)numFuncs * M68K_JT_SLOT_SIZE;
    /* The size is the file's. A base that cannot lie in containment is
       refused here, before the sum below can wrap and before anything is
       cleared with that size. */
    if (jt_size > ctx->mem_size || dataSize > ctx->mem_size - jt_size)
    {
        m68k_log(ctx, "68k library %s: a base of %lu bytes does not fit", name, (unsigned long)dataSize);
        return m68k_library_abandon(ctx, entry, 0, 0);
    }
    total = jt_size + dataSize;
    region = M68KEmu_HeapAlloc(ctx, total, 0);
    if (!region) return m68k_library_abandon(ctx, entry, 0, 0);
    /* The base is cleared and written below without the accessors. Where
       the block lies is the heap's word, and the heap keeps its lists in
       memory that a 68k program can write: what is not a span of
       containment is not used, and not given back as if it were a block. */
    if (!m68k_in_mem(ctx, region, total))
    {
        m68k_log(ctx, "68k library %s: the heap gave a base outside memory", name);
        return m68k_library_abandon(ctx, entry, 0, 0);
    }

    base = region + jt_size;  /* base is after the jump table */

    /* Build jump table: JMP abs.l for each function */
    m68k_put_vectors(ctx, base, vectors, rel, funcTable, numFuncs);

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

    if (dataInit && !m68k_init_struct(ctx, dataInit, base, dataSize))
    {
        m68k_log(ctx, "68k library %s: its init table is not usable", name);
        return m68k_library_abandon(ctx, entry, region, total);
    }

    /* As exec does, the library's init function runs, then its Open
       vector; both as 68k code when the program's OpenLibrary returns, and
       the result of Open is the OpenLibrary result. The caller's A6 is
       kept. Check the actual version after init, before Open. An initialized
       library remains reusable if the first version request is rejected.
       A later OpenLibrary calls only Open. */
    {
        static const UWORD first[] = {
            0x2f0e,                     /* move.l a6,-(sp) */
            0x4eb9, 0, 0,               /* jsr init */
            0x4a80,                     /* tst.l d0 */
            0x6716,                     /* beq.s done */
            0x2c40,                     /* movea.l d0,a6 */
            0x203c, 0, 0,               /* move.l #version,d0 */
            0xb06e, M68K_LIB_VERSION,    /* cmp.w lib_Version(a6),d0 */
            0x6206,                     /* bhi.s reject (unsigned) */
            0x4eae, 0xfffa,             /* jsr -6(a6) */
            0x6002,                     /* bra.s done */
            0x7000,                     /* reject: moveq #0,d0 */
            0x2c5f,                     /* done: movea.l (sp)+,a6 */
            0x4e75 };                   /* rts */
        static const UWORD again[] = {
            0x2f0e,                     /* move.l a6,-(sp) */
            0x2c7c, 0, 0,               /* movea.l #base,a6 */
            0xb06e, M68K_LIB_VERSION,    /* cmp.w lib_Version(a6),d0 */
            0x6206,                     /* bhi.s reject (unsigned) */
            0x4eae, 0xfffa,             /* jsr -6(a6) */
            0x6002,                     /* bra.s done */
            0x7000,                     /* reject: moveq #0,d0 */
            0x2c5f,                     /* done: movea.l (sp)+,a6 */
            0x4e75 };                   /* rts */
        ULONG t = M68KEmu_HeapAlloc(ctx, sizeof(first) + sizeof(again), 0), r;

        if (!t)
            return m68k_library_abandon(ctx, entry, region, total);
        r = t + sizeof(first);
        for (ULONG i = 0; i < sizeof(first) / sizeof(first[0]); i++)
            m68k_write16(ctx, t + 2 * i, first[i]);
        for (ULONG i = 0; i < sizeof(again) / sizeof(again[0]); i++)
            m68k_write16(ctx, r + 2 * i, again[i]);
        m68k_write32(ctx, t + 4, initFunc);
        m68k_write32(ctx, t + 16, version);
        m68k_write32(ctx, r + 4, base);
        m68k_engine_invalidate(ctx);       /* the jump table and the two pieces of 68k code above */
        ts = m68k_ts(ctx);
        if (initFunc)
            ts->tail_pc = t;
        else
        {
            ts->tail_pc = r;
            ts->tail_d0 = version;
        }
        if (initFunc)
            ts->tail_d0 = base;
        ts->tail_a0 = 0;

        if (ctx->num_m68klibs < sizeof(ctx->m68klibs) / sizeof(ctx->m68klibs[0]))
        {
            snprintf(ctx->m68klibs[ctx->num_m68klibs].name, sizeof(ctx->m68klibs[0].name), "%s", (const char *)FilePart((STRPTR)name));
            ctx->m68klibs[ctx->num_m68klibs].base = base;
            ctx->m68klibs[ctx->num_m68klibs].reopen = r;
            ctx->num_m68klibs++;
        }
    }

    if (ctx->trace_calls) bug("[m68kemu] m68klib: %s loaded at base=0x%lx (%d funcs)\n",
        name, (unsigned long)base, numFuncs);

    return base;
}

/* exec MakeFunctions() for 68k code: the jump table below target, from a
   table of addresses, or of WORD displacements from disp when disp is not
   0. The count slots written take count * 6 bytes. The result is what
   exec's MakeFunctions() returns: makefunctions.c measures from target to
   the slot below the last one written, (count + 1) * 6, 6 for an empty
   table. 0, with nothing written, when the table or the slots below target
   are not in containment. */
ULONG M68KEmu_MakeFunctions(struct M68KEmuContext *ctx, ULONG target, ULONG array, ULONG disp)
{
    LONG count = m68k_count_vectors(ctx, array, disp != 0);
    ULONG size;

    if (count < 0 || count > M68K_MAX_VECTORS)
    {
        m68k_log(ctx, "MakeFunctions: the function table at 0x%lx has no end in memory", (unsigned long)array);
        return 0;
    }
    size = (ULONG)count * M68K_JT_SLOT_SIZE;
    if (target < size || !m68k_in_mem(ctx, target - size, size))
    {
        m68k_log(ctx, "MakeFunctions: a jump table of %lu bytes below 0x%lx is outside memory",
                 (unsigned long)size, (unsigned long)target);
        return 0;
    }
    m68k_put_vectors(ctx, target, array, disp != 0, disp, count);
    m68k_engine_invalidate(ctx);
    return size + M68K_JT_SLOT_SIZE;
}

/* exec MakeLibrary() for 68k code. As exec does: the jump table, from a
   table of addresses or, when it starts with the WORD -1, of displacements
   from it; lib_NegSize, the table's size rounded up to a LONG, and
   lib_PosSize, dataSize; the InitStruct() table, when there is one; then
   the library's init, when there is one, with D0 the base, A0 segList and
   A6 SysBase, and what it returns is the result. Init runs as 68k code when
   the caller's call returns, and its RTS returns to the caller.
   The base and its jump table are one block of the program's heap, cleared,
   as AllocMem() from 68k code gives one: the library's Expunge gives it back
   with FreeMem() of lib_NegSize + lib_PosSize bytes at base - lib_NegSize.
   0 when the table is not in containment, when init is odd or not in
   containment (exec would jump there), when dataSize is smaller than a
   Library (exec would write past the block), when the heap has no block,
   or when the InitStruct() table does not fit in the base (exec would write
   past it); a block allocated already is given back then. */
ULONG M68KEmu_MakeLibrary(struct M68KEmuContext *ctx, ULONG funcInit, ULONG structInit, ULONG libInit,
                          ULONG dataSize, ULONG segList)
{
    ULONG vectors, negsize, total, region, base;
    LONG count;
    BOOL rel;

    if (!m68k_in_mem(ctx, funcInit, 2))
    {
        m68k_log(ctx, "MakeLibrary: the function table at 0x%lx is outside memory", (unsigned long)funcInit);
        return 0;
    }
    rel = m68k_read16(ctx, funcInit) == 0xFFFF;
    vectors = rel ? funcInit + 2 : funcInit;
    count = m68k_count_vectors(ctx, vectors, rel);
    if (count < 0 || count > M68K_MAX_VECTORS)
    {
        m68k_log(ctx, "MakeLibrary: the function table at 0x%lx has no end in memory", (unsigned long)funcInit);
        return 0;
    }
    if (libInit && ((libInit & 1) || !m68k_in_mem(ctx, libInit, 2)))
    {
        m68k_log(ctx, "MakeLibrary: the init at 0x%lx is not 68k code in memory", (unsigned long)libInit);
        return 0;
    }
    negsize = ((ULONG)count * M68K_JT_SLOT_SIZE + 3) & ~3u;
    /* lib_PosSize is a UWORD. A larger allocation could not be freed using
       the size published in the library header. */
    if (dataSize < M68K_SIZEOF_LIBRARY || dataSize > 0xFFFFu ||
        negsize > ctx->mem_size || dataSize > ctx->mem_size - negsize)
    {
        m68k_log(ctx, "MakeLibrary: a base of %lu bytes is not usable", (unsigned long)dataSize);
        return 0;
    }
    total = negsize + dataSize;
    region = M68KEmu_HeapAlloc(ctx, total, MEMF_PUBLIC | MEMF_CLEAR);
    if (!region)
        return 0;
    /* what is not a span of containment is not used, and not given back */
    if (!m68k_in_mem(ctx, region, total))
    {
        m68k_log(ctx, "MakeLibrary: the heap gave a base outside memory");
        return 0;
    }
    memset(ctx->mem + region, 0, total);
    base = region + negsize;

    m68k_put_vectors(ctx, base, vectors, rel, funcInit, count);
    m68k_engine_invalidate(ctx);       /* the jump table, kept or given back below */
    m68k_write16(ctx, base + M68K_LIB_NEGSIZE, (UWORD)negsize);
    m68k_write16(ctx, base + M68K_LIB_POSSIZE, (UWORD)dataSize);
    if (structInit && !m68k_init_struct(ctx, structInit, base, dataSize))
    {
        m68k_log(ctx, "MakeLibrary: the InitStruct() table at 0x%lx does not fit in the base", (unsigned long)structInit);
        M68KEmu_HeapFree(ctx, region, total);
        return 0;
    }

    if (libInit)
    {
        struct M68KTaskState *ts = m68k_ts(ctx);

        ts->tail_pc = libInit;
        ts->tail_d0 = base;
        ts->tail_a0 = segList;
        ts->tail_a6 = ctx->m68k_sysbase;
    }
    return base;
}
