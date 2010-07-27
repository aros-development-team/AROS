#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <stdarg.h>
#include <string.h>

#include "debug_elf.h"
#include "debug_intern.h"
#include "kernel_intern.h"

#define D(x)
#define DSYMS(x)

AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    int res;

    /* Windows console output aborts if task switch occurs while it's running */
    if (SysBase)
    	Forbid();
    res = HostIFace->VKPrintF(format, args);
    if (SysBase)
    	Permit();
    return res;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH4(void, KrnRegisterModule,
	 AROS_LHA(const char *, name, A0),
	 AROS_LHA(BPTR, segList, A1),
	 AROS_LHA(ULONG, debugType, D0),
	 AROS_LHA(APTR, debugInfo, A2),
	 struct KernelBase *, KernelBase, 21, Kernel)
{
    AROS_LIBFUNC_INIT

    /* TODO: rewrite the thing to support multiple sections of one module */

    D(bug("[KRN] KrnRegisterModule(%s, 0x%p, %d)\n", name, segList, debugType));

    if (debugType == DEBUG_ELF) {
    	struct elfheader *eh = ((struct ELF_DebugInfo *)debugInfo)->eh;
	struct sheader *sections = ((struct ELF_DebugInfo *)debugInfo)->sh;
	module_t *mod = AllocVec(sizeof(module_t) + strlen(name), MEMF_PUBLIC | MEMF_CLEAR);

	if (mod) {
	    int i;

	    D(bug("[KRN] %d sections at 0x%p\n", eh->int_shnum, sections));

	    strcpy(mod->m_name, name);
	    NEWLIST(&mod->m_symbols);
	    mod->segList = BADDR(segList);
	    mod->m_lowest = (void *)-1;
	    mod->m_highest = NULL;

	    for (i=0; i < eh->int_shnum; i++) {
		/* If we have string table, copy it */
		if (sections[i].type == SHT_STRTAB && i != SHINDEX(eh->int_shstrndx)) {
		    DSYMS(bug("[KRN]  symbol table of length %d in section %d\n", sections[i].size, i));

		    if (!mod->m_str) {
			mod->m_str = AllocVec(sections[i].size, MEMF_PUBLIC | MEMF_CLEAR);
			CopyMem(sections[i].addr, mod->m_str, sections[i].size);
		    }
		}

		if ((sections[i].flags & (SHF_ALLOC | SHF_EXECINSTR)) == (SHF_ALLOC | SHF_EXECINSTR)) {
		    if (sections[i].addr) {
			if (sections[i].addr < mod->m_lowest)
			    mod->m_lowest = sections[i].addr;
			if (sections[i].addr + sections[i].size > mod->m_highest)
			    mod->m_highest = sections[i].addr + sections[i].size;
		    }
		}
	    }

	    for (i=0; i < eh->int_shnum; i++) {
		if (sections[i].addr && sections[i].type == SHT_SYMTAB) {
		    int j;
		    struct symbol *st = (struct symbol *)sections[i].addr;

		    for (j=0; j < (sections[i].size / sizeof(struct symbol)); j++) {
			if (st[j].shindex != SHN_XINDEX) {
			    if (sections[st[j].shindex].addr && (sections[st[j].shindex].flags & (SHF_ALLOC | SHF_EXECINSTR)) == (SHF_ALLOC | SHF_EXECINSTR)) {
				symbol_t *sym = AllocVec(sizeof(symbol_t), MEMF_PUBLIC | MEMF_CLEAR);

				sym->s_name = &mod->m_str[st[j].name];
				sym->s_lowest = sections[st[j].shindex].addr + st[j].value;
				sym->s_highest = sym->s_lowest + st[j].size;

				DSYMS(bug("[KRN]  Adding symbol '%s' %08x-%08x\n", sym->s_name, sym->s_lowest, sym->s_highest-1));

				AddHead((struct List *)&mod->m_symbols, (struct Node *)sym);
			    }
			}
		    }

		    break;
		}
	    }

	    D(bug("[KRN]  address range %08x - %08x\n", mod->m_lowest, mod->m_highest-1));
	    ObtainSemaphore(&KernelBase->kb_ModSem);
	    AddHead((struct List *)&KernelBase->kb_Modules, (struct Node *)mod);
	    ReleaseSemaphore(&KernelBase->kb_ModSem);
	}
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnUnregisterModule,
		AROS_LHA(BPTR, segList, A0),
		struct KernelBase *, KernelBase, 22, Kernel)
{
    AROS_LIBFUNC_INIT
    
    module_t *mod;
    void *addr = BADDR(segList);

    D(bug("[KRN] KrnUnregisterModule(0x%p)\n", segList));
    ObtainSemaphore(&KernelBase->kb_ModSem);

    ForeachNode(&KernelBase->kb_Modules, mod) {
	if (mod->segList == addr) {
	    symbol_t *sym, *sym2;

	    D(bug("[KRN] Removing module %s\n", mod->m_name));
	    Remove(mod);

	    /* Free associated string table */
	    if (mod->m_str)
		FreeVec(mod->m_str);

	    /* Free associated symbols */
	    ForeachNodeSafe(&mod->m_symbols, sym, sym2)
		FreeVec(sym);

	    /* Free module node at last */
	    FreeVec(mod);

	    break;
	}
    }

    ReleaseSemaphore(&KernelBase->kb_ModSem);

    AROS_LIBFUNC_EXIT
}
