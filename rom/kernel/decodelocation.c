#include <kernel_base.h>
#include <kernel_tagitems.h>

#include "debug_intern.h"

#define SetModName(name) if (module) *module = name;
#define SetFuncName(name) if (function) *function = name;
#define SetSegOffset(val) if (soffset) *soffset = val;
#define SetFuncOffset(val) if (foffset) *foffset = val;

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(int, KrnDecodeLocation,

/*  SYNOPSIS */
	AROS_LHA(void *, addr, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct KernelBase *, KernelBase, 24, Kernel)

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
    symbol_t *sym;
    char **module = (char **)krnGetTagData(KDL_ModuleName, 0, tags);
    char **function = (char **)krnGetTagData(KDL_FunctionName, 0, tags);
    intptr_t *soffset = (intptr_t *)krnGetTagData(KDL_SegmentOffset, 0, tags);
    intptr_t *foffset = (intptr_t *)krnGetTagData(KDL_FunctionOffset, 0, tags);

    ForeachNode(&KernelBase->kb_Modules, mod)
    {
	/* if address suits the module bounds, you got it */
	if (mod->m_lowest <= addr && mod->m_highest > addr) {
	    SetModName(mod->m_name);
	    SetSegOffset(addr - mod->m_lowest); /* FIXME */

	    /* Now look up the function if requested */
	    if (function || foffset) {
		ForeachNode(&mod->m_symbols, sym) {
		    if (sym->s_lowest <= addr && sym->s_highest > addr) {
			SetFuncName(sym->s_name);
			SetFuncOffset(addr - sym->s_lowest);
			return 1;
		    }
		}
		SetFuncName(NULL);
		SetFuncOffset(-1);
	    }
	    return 1;
	}
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
