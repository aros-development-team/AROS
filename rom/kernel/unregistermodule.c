#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
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
	Remove previously registered module from the debug information database

    INPUTS
	segList - DOS segment list for the module to remove

    RESULT
	None

    NOTES
	The function correctly supports partial removal of the module
	(when an existing seglist is broken and only a part of the module
	is unloaded).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct segment *seg;

    D(bug("[KRN] KrnUnregisterModule(0x%p)\n", segList));
    ObtainSemaphore(&KernelBase->kb_ModSem);

    while (segList)
    {
	ForeachNode(&KernelBase->kb_Modules, seg) {
	    if (seg->s_seg == segList)
	    {
		module_t *mod = seg->s_mod;

	        D(bug("[KRN] Removing segment 0x%p\n", segList));
		Remove((struct Node *)seg);

		/* If module's segment count reached 0, remove the whole
		   module information */
		if (--mod->m_segcnt == 0)
		{
		    D(bug("[KRN] Removing module %s\n", mod->mod.m_name));

		    /* Free associated symbols */
		    if (mod->mod.m_symbols) {
			D(bug("[KRN] Removing symbol table 0x%p\n", mod->mod.m_symbols));
			FreeVec(mod->mod.m_symbols);
		    }

		    /* Free associated string tables */
		    if (mod->m_str) {
			D(bug("[KRN] Removing symbol name table 0x%p\n", mod->m_str));
			FreeVec(mod->m_str);
		    }
		    if (mod->m_shstr) {
			D(bug("[KRN] Removing section name table 0x%p\n", mod->m_str));
			FreeVec(mod->m_shstr);
		    }

		    /* Free module descriptor at last */
		    FreeVec(mod);
		}

		FreeMem(seg, sizeof(struct segment));

		break;
	    }
	}
	/* Advance to next DOS segment */
	segList = *(BPTR *)BADDR(segList);
    }

    ReleaseSemaphore(&KernelBase->kb_ModSem);

    AROS_LIBFUNC_EXIT
}
