/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#define DEBUG 0
#define DSEGS(x)

#include <aros/debug.h>
#include <dos/bptr.h>
#include <libraries/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <inttypes.h>

#include "debug_intern.h"

/* Binary search over sorted array of segments */
static struct segment * FindSegmentInModule(void *addr, module_t *mod)
{
    LONG idx, minidx = 0, maxidx = mod->m_segcnt - 1;

    while(TRUE)
    {
        idx = (maxidx + minidx) / 2;

        if (mod->m_segments[idx]->s_lowest <= addr)
        {
            if (mod->m_segments[idx]->s_highest >= addr)
            {
                return mod->m_segments[idx];
            }
            else
            {
                /* cut off segments with lower addresses */
                minidx = idx + 1;
            }
        }
        else
        {
            /* cut off segments with higher addresses */
            maxidx = idx - 1;
        }

        /* Not found, aborting */
        if (maxidx < minidx)
            return NULL;
    }

    return NULL;
}

static struct segment * FindSegment(void *addr, struct Library *DebugBase)
{
    module_t * mod;

    ForeachNode(&DBGBASE(DebugBase)->db_Modules, mod)
    {
        DSEGS(bug("[Debug] Checking module 0x%p - 0x%p, %s\n", mod->m_lowest, mod->m_highest, mod->m_name));

        /* if address suits the module bounds, you got a candidate */
        if ((mod->m_lowest <= addr) && (mod->m_highest >= addr))
        {
            struct segment *seg = FindSegmentInModule(addr, mod);
            if (seg)
                return seg;
        }
    }

    return NULL;
}

static BOOL FindSymbol(module_t *mod, char **function, void **funstart, void **funend, void *addr)
{
    dbg_sym_t *sym = mod->m_symbols;
    unsigned int i;

    /* Caller didn't care about symbols? */
    if (!addr)
        return TRUE;

    for (i = 0; i < mod->m_symcnt; i++)
    {
        APTR highest = sym[i].s_highest;

        /* Symbols with zero length have zero in s_highest */
        if (!highest)
            highest = sym[i].s_lowest;

        if (sym[i].s_lowest <= addr && highest >= addr) {
            *function = sym[i].s_name;
            *funstart = sym[i].s_lowest;
            *funend   = sym[i].s_highest;

            return TRUE;
        }
    }

    /* Indicate that symbol not found */
    return FALSE;
}

/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH2(int, DecodeLocationA,

/*  SYNOPSIS */
        AROS_LHA(void *, addr, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, DebugBase, 7, Debug)

/*  FUNCTION
        Locate the given address in the list of registered modules and return
        information about it.

    INPUTS
        addr - An address to resolve
        tags - An optional taglist. ti_Tag can be one of the following tags and
               ti_Data is always a pointer to a storage of specified type.
               Resulting values will be placed into specified locations if the
               function succeeds.

            DL_ModuleName     (char *) - Module name
            DL_SegmentName    (char *) - Segment name. Can be NULL if there were
                                         no segment names provided for the module.
            DL_SegmentPointer (BPTR)   - DOS pointer to the corresponding segment.
                                         Note that it will be different from
                                         KDL_SegmentStart value
            
            DL_SegmentNumber  (unsigned int) - Order number of the segment in the
                                               module
            DL_SegmentStart   (void *) - Start address of actual segment contents
                                         in memory.
            DL_SegmentEnd     (void *) - End address of actual segment contents
                                         in memory.
            DL_FirstSegment   (BPTR) - DOS pointer to the first segment.

            The following tags may return NULL values if there was no corresponding
            information provided for the module:

            DL_SymbolName     (char *) - Symbol name (function or variable name)
            DL_SymbolStart    (void *) - Start address of contents described by this
                                         symbol.
            DL_SymbolEnd      (void *) - End address of contents described by this
                                         symbol.

    RESULT
        Zero if lookup failed and no corresponding module found, nonzero
        otherwise.

    NOTES
        If the function fails values pointed to by taglist will not be changed.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct segment *seg;
    void *dummy;
    char **module   = (char **)&dummy;
    char **segment  = (char **)&dummy;
    char **function = (char **)&dummy;
    void **secstart = &dummy;
    void **secend   = &dummy;
    void **funstart = &dummy;
    void **funend   = &dummy;
    BPTR  *secptr   = (BPTR *)&dummy;
    BPTR  *secfirst = (BPTR *)&dummy;
    unsigned int *secnum = (unsigned int *)&dummy;
    struct TagItem *tag, *tstate = tags;
    void *symaddr = NULL;
    int ret = 0;
    int super;

    D(bug("[Debug] DecodeLocationA(0x%p)\n", addr));

    /* Parse TagList */
    while ((tag = LibNextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case DL_ModuleName:
            module = (char **)tag->ti_Data;
            break;

        case DL_SegmentName:
            segment = (char **)tag->ti_Data;
            break;

        case DL_SegmentPointer:
            secptr = (BPTR *)tag->ti_Data;
            break;

        case DL_SegmentNumber:
            secnum = (unsigned int *)tag->ti_Data;
            break;

        case DL_SegmentStart:
            secstart = (void **)tag->ti_Data;
            break;

        case DL_SegmentEnd:
            secend = (void **)tag->ti_Data;
            break;

        case DL_FirstSegment:
            secfirst = (BPTR *)tag->ti_Data;
            break;

        case DL_SymbolName:
            function = (char **)tag->ti_Data;
            symaddr = addr;
            break;

        case DL_SymbolStart:
            funstart = (void **)tag->ti_Data;
            symaddr = addr;
            break;

        case DL_SymbolEnd:
            funend = (void **)tag->ti_Data;
            symaddr = addr;
            break;
        }
    }

    /* We can be called in supervisor mode. No semaphores in the case! */
    super = KrnIsSuper();
    if (!super)
        ObtainSemaphoreShared(&DBGBASE(DebugBase)->db_ModSem);

    seg = FindSegment(addr, DebugBase);
    if (seg)
    {
        D(bug("[Debug] Found module %s, Segment %u (%s, 0x%p - 0x%p)\n", seg->s_mod->m_name, seg->s_num,
               seg->s_name, seg->s_lowest, seg->s_highest));

        *module   = seg->s_mod->m_name;
        *segment  = seg->s_name;
        *secptr   = seg->s_seg;
        *secnum   = seg->s_num;
        *secstart = seg->s_lowest;
        *secend   = seg->s_highest;
        *secfirst = seg->s_mod->m_seg;

        /* Now look up the function if requested */
        FindSymbol(seg->s_mod, function, funstart, funend, symaddr);
        ret = 1;
    }

    if (!super)
        ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    return ret;

    AROS_LIBFUNC_EXIT
}
