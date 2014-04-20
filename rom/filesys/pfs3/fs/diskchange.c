#define __USE_SYSBASE
#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>

#include "blocks.h"
#include "struct.h"

#include <proto/exec.h>

#ifdef __MORPHOS__

#include <emul/emulregs.h>

static ULONG DiskChangeHandler_native(void)
{
	struct globaldata *g = (void *) REG_A1;
	Signal(&g->myproc->pr_Task, g->diskchangesignal);
	return 0;
}

const struct EmulLibEntry DiskChangeHandler __READONLY__ =
{
	TRAP_LIB,
	0,
	(void (*)(void)) DiskChangeHandler_native
};

#elif __AROS__

#undef SysBase

AROS_UFH2(ULONG, DiskChangeHandler,
	AROS_UFHA(struct globaldata *, g, A1),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{ 
	AROS_USERFUNC_INIT

	Signal(&g->myproc->pr_Task, g->diskchangesignal);
	return 0;

	AROS_USERFUNC_EXIT
}

#else

#warning "Add the implementation for your platform"

#endif
