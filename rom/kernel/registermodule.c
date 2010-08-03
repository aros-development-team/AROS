#include <aros/libcall.h>
#include <dos/dosextens.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "debug_intern.h"

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x)
#define DSYMS(x)

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH4(void, KrnRegisterModule,

/*  SYNOPSIS */
	AROS_LHA(const char *, name, A0),
	AROS_LHA(BPTR, segList, A1),
	AROS_LHA(ULONG, debugType, D0),
	AROS_LHA(APTR, debugInfo, A2),

/*  LOCATION */
	struct KernelBase *, KernelBase, 22, Kernel)

/*  FUNCTION

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

    D(bug("[KRN] KrnRegisterModule(%s, 0x%p, %d)\n", name, segList, debugType));

    if (debugType == DEBUG_ELF) {
    	struct elfheader *eh = ((struct ELF_DebugInfo *)debugInfo)->eh;
	struct sheader *sections = ((struct ELF_DebugInfo *)debugInfo)->sh;
	module_t *mod = AllocVec(sizeof(module_t) + strlen(name), MEMF_PUBLIC);

	if (mod) {
	    int i;

	    D(bug("[KRN] %d sections at 0x%p\n", eh->int_shnum, sections));

	    strcpy(mod->m_name, name);
	    NEWLIST(&mod->m_symbols);
	    mod->m_str = NULL;
	    mod->m_segcnt = 0;

	    for (i=0; i < eh->int_shnum; i++) {
		/* If we have string table, copy it */
		if (sections[i].type == SHT_STRTAB && i != SHINDEX(eh->int_shstrndx))
		{
		    D(bug("[KRN] string table of length %d in section %d\n", sections[i].size, i));

		    if (!mod->m_str) {
			mod->m_str = AllocVec(sections[i].size, MEMF_PUBLIC);
			CopyMem(sections[i].addr, mod->m_str, sections[i].size);
		    }
		}

		if (segList && (sections[i].flags & SHF_ALLOC))
		{
		    /* Actually register only executable sections */
		    if (sections[i].flags & SHF_EXECINSTR)
		    {
			struct segment *seg = AllocMem(sizeof(struct segment), MEMF_PUBLIC);

			if (seg) {
			    D(bug("[KRN] Adding segment 0x%p\n", segList));

			    seg->s_lowest  = sections[i].addr;
			    seg->s_highest = sections[i].addr + sections[i].size - 1;
			    seg->s_seg     = segList; /* Note that this will differ from s_lowest */
			    seg->s_mod     = mod;
			    seg->s_num     = i;
			    seg->s_name    = NULL; /* TODO: extract segment name */

			    mod->m_segcnt++;
			    ObtainSemaphore(&KernelBase->kb_ModSem);
			    AddTail((struct List *)&KernelBase->kb_Modules, (struct Node *)seg);
			    ReleaseSemaphore(&KernelBase->kb_ModSem);
			}
		    }

		    /* Advance to next DOS segment */
		    segList = *(BPTR *)BADDR(segList);
		}
	    }

	    /* If the module contains no executable segments (hm, weird),
	       we actually got nothing linked in our base list. This means
	       that module handle is actually a garbage and we can deallocate
	       it right now, and do nothing more */
	    if (mod->m_segcnt == 0)
	    {
		if (mod->m_str)
		    FreeVec(mod->m_str);
		FreeVec(mod);

		return;
	    }

	    /* Parse module's symbol table */
	    for (i=0; i < eh->int_shnum; i++) {
		if (sections[i].addr && sections[i].type == SHT_SYMTAB) {
		    int j;
		    struct symbol *st = (struct symbol *)sections[i].addr;

		    for (j=0; j < (sections[i].size / sizeof(struct symbol)); j++) {
			if (st[j].shindex != SHN_XINDEX) {
			    if (sections[st[j].shindex].addr && (sections[st[j].shindex].flags & (SHF_ALLOC | SHF_EXECINSTR)) == (SHF_ALLOC | SHF_EXECINSTR)) {
				symbol_t *sym = AllocMem(sizeof(symbol_t), MEMF_PUBLIC);

				if (mod->m_str)
				    sym->s_name = &mod->m_str[st[j].name];
				else
				    sym->s_name = NULL;
				sym->s_lowest = sections[st[j].shindex].addr + st[j].value;
				sym->s_highest = sym->s_lowest + st[j].size - 1;

				DSYMS(bug("[KRN]  Adding symbol '%s' %08x-%08x\n", sym->s_name, sym->s_lowest, sym->s_highest));
				AddHead((struct List *)&mod->m_symbols, (struct Node *)sym);
			    }
			}
		    }

		    break;
		}
	    }
	}
    }

    AROS_LIBFUNC_EXIT
}
