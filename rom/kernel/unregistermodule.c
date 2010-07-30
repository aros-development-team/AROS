#include <aros/libcall.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "debug_elf.h"
#include "debug_intern.h"

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x)

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH1(void, KrnUnregisterModule,

/*  SYNOPSIS */
	AROS_LHA(BPTR, segList, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 23, Kernel)

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
    
    module_t *mod;
    void *addr = BADDR(segList);

    D(bug("[KRN] KrnUnregisterModule(0x%p)\n", segList));
    ObtainSemaphore(&KernelBase->kb_ModSem);

    ForeachNode(&KernelBase->kb_Modules, mod) {
	if (mod->segList == addr) {
	    symbol_t *sym, *sym2;

	    D(bug("[KRN] Removing module %s\n", mod->m_name));
	    Remove((struct Node *)mod);

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
