#include <exec/execbase.h>
#include <proto/exec.h>
#include <aros/asmcall.h>
#include "exec_private.h"

/* All I need is a global variable SysBase */
extern struct ExecBase *SysBase;

AROS_UFH2 (void, KPutChar,
	AROS_UFHA(UBYTE, chr, D0),
	AROS_UFHA(struct ExecBase *,SysBase,A3)
)
{
    AROS_LIBFUNC_INIT
    RawPutChar(chr);
    AROS_LIBFUNC_EXIT
}

void KPrintF(STRPTR format, ...)
{
    RawDoFmt(format,&format+1,(VOID_FUNC)KPutChar,SysBase);
}
