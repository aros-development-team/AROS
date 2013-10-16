/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#define DEBUG 0
#define DSYMS(x)

#include <aros/debug.h>
#include <aros/libcall.h>
#include <dos/dosextens.h>
#include <exec/lists.h>
#include <libraries/debug.h>
#include <proto/exec.h>

#include <stdint.h>
#include <string.h>

#include "debug_intern.h"

static inline char *getstrtab(struct sheader *sh)
{
    char *str;

    str = AllocVec(sh->size, MEMF_PUBLIC);
    if (str)
        CopyMem(sh->addr, str, sh->size);

    return str;
}

static void addsymbol(module_t *mod, dbg_sym_t *sym, struct symbol *st, APTR value)
{
    if (mod->m_str)
	sym->s_name = &mod->m_str[st->name];
    else
	sym->s_name = NULL;

    sym->s_lowest = value;
    if (st->size)
	sym->s_highest = value + st->size - 1;
    else
	/* For symbols with zero size KDL_SymbolEnd will give NULL */ 
	sym->s_highest = NULL;

    /* We count symbols here because not all of them can be added */
    mod->m_symcnt++;
}

static void RegisterModule_Hunk(const char *name, BPTR segList, ULONG DebugType, APTR DebugInfo, struct Library *DebugBase)
{
    module_t *mod;
    int i = 0;

    mod = AllocVec(sizeof(module_t) + strlen(name), MEMF_PUBLIC|MEMF_CLEAR);
    if (!mod)
        return;
    strcpy(mod->m_name, name);
    mod->m_seg = segList;
    while (segList) {
        ULONG *segPtr = BADDR(segList);
        struct segment *seg = AllocMem(sizeof(struct segment), MEMF_PUBLIC | MEMF_CLEAR);
        if (seg) {
            seg->s_lowest  = (UBYTE*)segPtr - 4;
            seg->s_highest = (UBYTE*)segPtr + segPtr[-1];
            seg->s_seg     = segList;
            seg->s_num     = i;
            seg->s_mod     = mod;
            mod->m_segcnt++;
            ObtainSemaphore(&DBGBASE(DebugBase)->db_ModSem);
            AddTail((struct List *)&DBGBASE(DebugBase)->db_Modules, (struct Node *)seg);
            ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);
            D(bug("[Debug] Adding segment %d 0x%p (%p-%p)\n", i, segList, seg->s_lowest, seg->s_highest));
        }
        segList = *(BPTR *)BADDR(segList);
        i++;
    }
}


/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH4(void, RegisterModule,

/*  SYNOPSIS */
	AROS_LHA(const char *, name, A0),
	AROS_LHA(BPTR, segList, A1),
	AROS_LHA(ULONG, debugType, D0),
	AROS_LHA(APTR, debugInfo, A2),

/*  LOCATION */
	struct Library *, DebugBase, 5, Debug)

/*  FUNCTION
	Add information about the loaded executable module to the
	debug information database

    INPUTS
	name      - Module name
	segList   - DOS segment list for the module
	debugType - Type of supplied debug information. The only currently
		    supported type is DEBUG_ELF.
	debugInfo - Debug information data. For DEBUG_ELF type this should be
		    a pointer to struct ELF_DebugInfo, filled in as follows:
		      eh - a pointer to ELF file header.
		      sh - a pointer to an array of ELF section headers.

    RESULT
	None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	The function supposes that segments in DOS list are linked in the same
	order in which corresponding sections are placed in the file

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[Debug] RegisterModule(%s, 0x%p, %d)\n", name, segList, debugType));

    if (debugType == DEBUG_ELF)
    {
    	struct elfheader *eh = ((struct ELF_DebugInfo *)debugInfo)->eh;
	struct sheader *sections = ((struct ELF_DebugInfo *)debugInfo)->sh;
	module_t *mod = AllocVec(sizeof(module_t) + strlen(name), MEMF_PUBLIC|MEMF_CLEAR);

	if (mod)
	{
	    ULONG int_shnum    = eh->shnum;
	    ULONG int_shstrndx = eh->shstrndx;
	    ULONG shstr;
	    ULONG i;

            /* Get wider versions of shnum and shstrndx from first section header if needed */
            if (int_shnum == 0)
            	int_shnum = sections[0].size;
            if (int_shstrndx == SHN_XINDEX)
            	int_shstrndx = sections[0].link;

	    D(bug("[Debug] %d sections at 0x%p\n", int_shnum, sections));
	    shstr = SHINDEX(int_shstrndx);

	    strcpy(mod->m_name, name);
	    mod->m_seg = segList;
	    if (sections[shstr].type == SHT_STRTAB)
		mod->m_shstr = getstrtab(&sections[shstr]);

	    for (i=0; i < int_shnum; i++)
	    {
		/* Ignore all empty segments */
		if (sections[i].size)
		{
		    /* If we have string table, copy it */
		    if ((sections[i].type == SHT_STRTAB) && (i != shstr) && (!mod->m_str)) {
			D(bug("[Debug] Symbol name table of length %d in section %d\n", sections[i].size, i));
			mod->m_str = getstrtab(&sections[i]);
		    }

		    /* Every loadable section with nonzero size got a corresponding DOS segment */
		    if (segList && (sections[i].flags & SHF_ALLOC))
		    {
			struct segment *seg = AllocMem(sizeof(struct segment), MEMF_PUBLIC);

			if (seg) {
			    D(bug("[Debug] Adding segment 0x%p\n", segList));

			    seg->s_lowest  = sections[i].addr;
			    seg->s_highest = sections[i].addr + sections[i].size - 1;
			    seg->s_seg     = segList; /* Note that this will differ from s_lowest */
			    seg->s_mod     = mod;
			    seg->s_num     = i;
			    if (mod->m_shstr)
				seg->s_name = &mod->m_shstr[sections[i].name];
			    else
				seg->s_name = NULL;

			    mod->m_segcnt++;

			    ObtainSemaphore(&DBGBASE(DebugBase)->db_ModSem);
			    AddTail((struct List *)&DBGBASE(DebugBase)->db_Modules, (struct Node *)seg);
			    ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);
			}

			/* Advance to next DOS segment */
			segList = *(BPTR *)BADDR(segList);
		    }
		}
	    }

	    /* If the module contains no loadable segments (hm, weird),
	       we actually got nothing linked in our base list. This means
	       that module handle is actually a garbage and we can deallocate
	       it right now, and do nothing more */
	    if (mod->m_segcnt == 0)
	    {
		FreeVec(mod->m_str);
		FreeVec(mod->m_shstr);
		FreeVec(mod);

		return;
	    }

	    /* Parse module's symbol table */
	    for (i=0; i < int_shnum; i++)
	    {
		if (sections[i].addr && sections[i].type == SHT_SYMTAB)
		{
		    struct symbol *st = (struct symbol *)sections[i].addr;
		    unsigned int symcnt = sections[i].size / sizeof(struct symbol);
		    dbg_sym_t *sym = AllocVec(sizeof(dbg_sym_t) * symcnt, MEMF_PUBLIC);
		    unsigned int j;

		    mod->m_symbols = sym;

		    if (sym) {
			for (j=0; j < symcnt; j++)
			{
			    int idx = st[j].shindex;

			    /* Ignore these - they should not be here at all */
			    if ((idx == SHN_UNDEF) || (idx == SHN_COMMON))
				continue;
			    /* TODO: perhaps XINDEX support is needed */
			    if (idx == SHN_XINDEX)
				continue;

			    if (idx == SHN_ABS) {
				addsymbol(mod, sym, &st[j], (APTR)st[j].value);
				DSYMS(bug("[Debug] Added ABS symbol '%s' %08x-%08x\n", sym->s_name, sym->s_lowest, sym->s_highest));
				sym++;
			    } else if (sections[idx].addr && (sections[idx].flags & SHF_ALLOC)) {
				addsymbol(mod, sym, &st[j], sections[idx].addr + st[j].value);
				DSYMS(bug("[Debug] Added symbol '%s' %08x-%08x\n", sym->s_name, sym->s_lowest, sym->s_highest));
				sym++;
			    }
			}
		    }
		    break;
		}
	    }
	}
    } else if (debugType == DEBUG_PARTHENOPE) {
        const struct Parthenope_ModuleInfo *pm;

        ForeachNode(debugInfo, pm) {
            struct segment *seg;
           
            seg = AllocVec(sizeof(struct segment) + sizeof(module_t) + strlen(pm->m_name), MEMF_PUBLIC|MEMF_CLEAR);

            if (seg) {
                unsigned int symbols;
                dbg_sym_t *dsym;
                const struct Parthenope_Symbol *sym;
                APTR str_l = (APTR)~(uintptr_t)0, str_h = (APTR)(uintptr_t)0;
                APTR seg_l = (APTR)~(uintptr_t)0, seg_h = (APTR)(uintptr_t)0;
                module_t *mod = (module_t *)(&seg[1]);

                DSYMS(bug("[Debug] Adding module @%p: %s\n", pm, pm->m_name));
                strcpy(mod->m_name, pm->m_name);

                seg->s_seg = BNULL;
                seg->s_mod = mod;
                seg->s_name = mod->m_name;
                seg->s_num = 0;

                mod->m_shstr = NULL;
                mod->m_str = NULL;
                mod->m_segcnt = 0;
               
                /* Determine the size of the string table */
                symbols = 0;
                ForeachNode(&pm->m_symbols, sym) {
                    symbols++;
                    if (sym->s_name) {
                        APTR end = (APTR)sym->s_name + strlen(sym->s_name) + 1 + 1;
                        if ((APTR)sym->s_name < str_l)
                            str_l = (APTR)sym->s_name;
                        if (end > str_h)
                            str_h = end;
                    }
                    if ((APTR)(uintptr_t)sym->s_lowest < seg_l)
                        seg_l = (APTR)(uintptr_t)sym->s_lowest;
                    if ((APTR)(uintptr_t)sym->s_highest > seg_h)
                        seg_h = (APTR)(uintptr_t)sym->s_highest;
                }

                if (symbols) {
                    DSYMS(bug("[Debug]   String table %p-%p (%u bytes)\n", str_l, str_h, (unsigned int)(str_h - str_l)));
                    DSYMS(bug("[Debug]   Symbols for %p-%p (%u symbols)\n", seg_l, seg_h, symbols));

                    seg->s_lowest = (APTR)seg_l;
                    seg->s_highest = (APTR)seg_h;

                    mod->m_symcnt = symbols;
                    mod->m_str = AllocVec(str_h - str_l, MEMF_PUBLIC);

                    if (mod->m_str) {
                        CopyMem(str_l, mod->m_str, str_h - str_l);

                        mod->m_symbols = AllocVec(sizeof(*mod->m_symbols) * symbols, MEMF_PUBLIC);
                        if (mod->m_symbols) {
                            dsym = mod->m_symbols;
                            ForeachNode(&pm->m_symbols, sym) {
                                dsym->s_name = ((APTR)sym->s_name - str_l) + mod->m_str;
                                dsym->s_lowest = (APTR)(uintptr_t)sym->s_lowest;
                                dsym->s_highest= (APTR)(uintptr_t)sym->s_highest;
                                dsym++;
                            }
                            ObtainSemaphore(&DBGBASE(DebugBase)->db_ModSem);
                            AddTail((struct List *)&DBGBASE(DebugBase)->db_Modules, (struct Node *)seg);
                            ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);
                            continue;
                        }

                        FreeVec(mod->m_str);
                    }

                    FreeVec(seg);
                }
            }
        }
    } else if (debugType == DEBUG_HUNK) {
        RegisterModule_Hunk(name, segList, debugType, debugInfo, DebugBase);
    }
    AROS_LIBFUNC_EXIT
}
