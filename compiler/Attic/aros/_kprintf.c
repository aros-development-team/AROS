#include <exec/execbase.h>
#include <clib/exec_protos.h>

/* All I need is a global variable SysBase */
extern struct ExecBase *SysBase;

__RA2(void,KPutChar,UBYTE,chr,D0,struct ExecBase *,SysBase,A3)
{
    __AROS_FUNC_INIT
    RawPutChar(chr);
    __AROS_FUNC_EXIT
}

void KPrintF(STRPTR format, ...)
{
    RawDoFmt(format,&format+1,(VOID_FUNC)KPutChar,SysBase);
}
