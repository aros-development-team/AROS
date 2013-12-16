/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
 */

#include <libraries/debug.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "debug_intern.h"

static void EnumerateLoadedModules(struct Hook * handler, struct Library * DebugBase);
static void EnumerateKickstartModules(struct Hook * handler, struct Library * DebugBase);

/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH2(void, EnumerateSymbolsA,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, handler, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, DebugBase, 8, Debug)

/*  FUNCTION
    Function will call the handler hook for all symbols from kickstart and
    loaded modules that match the given search criteria.

    The message that is passed to hook contains a pointer to struct SymbolInfo.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL super;

    EnumerateKickstartModules(handler, DebugBase);

    /* We can be called in supervisor mode. No semaphores in the case! */
    super = KrnIsSuper();
    if (!super)
        ObtainSemaphoreShared(&DBGBASE(DebugBase)->db_ModSem);

    EnumerateLoadedModules(handler, DebugBase);

    if (!super)
        ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    AROS_LIBFUNC_EXIT
}

static inline void callhook(struct Hook * handler, CONST_STRPTR modname, CONST_STRPTR symname,
        APTR start, APTR end)
{
    struct SymbolInfo sinfo = {0};

    sinfo.si_Size           = sizeof(struct SymbolInfo);
    sinfo.si_ModuleName     = modname;
    sinfo.si_SymbolName     = symname;
    sinfo.si_SymbolStart    = start;
    sinfo.si_SymbolEnd      = end;

    CALLHOOKPKT(handler, NULL, &sinfo);
}

static void EnumerateLoadedModules(struct Hook * handler, struct Library * DebugBase)
{
    struct segment *seg;

    ForeachNode(&DBGBASE(DebugBase)->db_Modules, seg)
    {
        module_t *mod = seg->s_mod;
        dbg_sym_t *sym = mod->m_symbols;
        ULONG i;

        D(bug("[Debug] Checking segment 0x%p - 0x%p, num %u, module %s\n",
                seg->s_lowest, seg->s_highest, seg->s_num, mod->m_name));

        for (i = 0; i < mod->m_symcnt; i++)
        {
            APTR highest = sym[i].s_highest;

            /* Symbols with zero length have zero in s_highest */
            if (!highest)
                highest = sym[i].s_lowest;

            callhook(handler, mod->m_name, sym[i].s_name, sym[i].s_lowest, sym[i].s_highest);
        }
    }
}

static void EnumerateKickstartModules(struct Hook * handler, struct Library * DebugBase)
{
    struct ELF_ModuleInfo *kmod;
    for (kmod = DBGBASE(DebugBase)->db_KernelModules; kmod; kmod = kmod->Next)
    {
        /* We understand only ELF here */
        if (kmod->Type == DEBUG_ELF)
        {
            struct elfheader *eh = kmod->eh;
            struct sheader *sections = kmod->sh;
            ULONG int_shnum = eh->shnum;
            ULONG int_shstrndx = eh->shstrndx;
            ULONG shstr;
            ULONG i;

            /* Get wider versions of shnum and shstrndx from first section header if needed */
            if (int_shnum == 0)
                int_shnum = sections[0].size;
            if (int_shstrndx == SHN_XINDEX)
                int_shstrndx = sections[0].link;

            shstr = SHINDEX(int_shstrndx);

            for (i = 0; i < int_shnum; i++)
            {
                APTR s_lowest = sections[i].addr;

                /* Ignore all empty segments */
                if (s_lowest && sections[i].size)
                {
                    struct symbol *st = (struct symbol *) sections[i].addr;
                    ULONG symcnt = sections[i].size / sizeof(struct symbol);
                    ULONG j, z;
                    STRPTR m_str = NULL;

                    /* Find symbols name table */
                    for (z = 0; z < int_shnum; z++)
                    {
                        if ((sections[z].type == SHT_STRTAB) && (z != shstr))
                        {
                            m_str = sections[z].addr;
                        }
                    }

                    if (!(sections[i].addr && sections[i].type == SHT_SYMTAB))
                        continue;

                    for (j = 0; j < symcnt; j++)
                    {
                        LONG idx = st[j].shindex;
                        APTR s_lowest, s_highest;
                        STRPTR s_name;

                        /* Ignore these - they should not be here at all */
                        if ((idx == SHN_UNDEF) || (idx == SHN_COMMON))
                            continue;
                        /* TODO: perhaps XINDEX support is needed */
                        if (idx == SHN_XINDEX)
                            continue;

                        s_name = (m_str) ? m_str + st[j].name : NULL;

                        s_lowest = (APTR) st[j].value;
                        if (idx != SHN_ABS)
                            s_lowest += (IPTR) sections[idx].addr;

                        if (st[j].size)
                            s_highest = s_lowest + st[j].size - 1;
                        else
                            s_highest = NULL;

                        callhook(handler, kmod->Name, s_name, s_lowest, s_highest);
                    }
                }
            }
        }
    }
}
