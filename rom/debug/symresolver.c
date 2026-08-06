/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Kernel symbol resolver backed by the module list.

    Registered with kernel.resource so KrnPrintBacktrace() and the trap
    handlers can name addresses. Runs in trap context, so the module
    list is walked without taking db_ModSem - a list changing mid-crash
    is survivable, a resolver that waits on a semaphore is not.
*/

#include <aros/kernel.h>
#include <proto/exec.h>

#define DEBUG_NOPRIVATEINLINE
#include "debug_intern.h"

static struct segment *ResolverFindSegment(struct DebugBase *debugBase,
                                           void *addr)
{
    module_t *mod;

    ForeachNode(&debugBase->db_Modules, mod)
    {
        if (!((mod->m_gaplowest <= addr) && (mod->m_gaphighest >= addr)) &&
            ((mod->m_lowest <= addr) && (mod->m_highest >= addr)))
        {
            LONG minidx = 0, maxidx = mod->m_segcnt - 1;

            while (minidx <= maxidx)
            {
                LONG idx = (maxidx + minidx) / 2;
                struct segment *seg = mod->m_segments[idx];

                if (seg->s_lowest <= addr)
                {
                    if (seg->s_highest >= addr)
                        return seg;
                    minidx = idx + 1;
                }
                else
                    maxidx = idx - 1;
            }
        }
    }

    return NULL;
}

LONG Debug_SymResolver(APTR priv, APTR addr, struct KrnSymInfo *out)
{
    struct DebugBase *debugBase = priv;
    struct segment *seg;
    module_t *mod;
    unsigned long i;

    seg = ResolverFindSegment(debugBase, addr);
    if (!seg)
        return 0;
    mod = seg->s_mod;

    out->mod_name = (STRPTR)mod->m_name;
    out->seg_name = (STRPTR)seg->s_name;
    out->seg_start = seg->s_lowest;
    out->seg_end = seg->s_highest;
    out->seg_bptr = seg->s_seg;
    out->seg_num = seg->s_num;
    out->sym_name = NULL;
    out->sym_start = NULL;
    out->sym_end = NULL;

    for (i = 0; i < mod->m_symcnt; i++)
    {
        dbg_sym_t *sym = &mod->m_symbols[i];
        void *highest = sym->s_highest ? sym->s_highest : sym->s_lowest;

        if (sym->s_lowest <= addr && highest >= addr)
        {
            out->sym_name = (STRPTR)sym->s_name;
            out->sym_start = sym->s_lowest;
            out->sym_end = sym->s_highest;
            break;
        }
    }

    return 1;
}
