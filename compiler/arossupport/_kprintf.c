/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level debugging support.
*/

#include <exec/execbase.h>
#include <proto/exec.h>
#include <aros/asmcall.h>
#include <stdarg.h>

/* All I need is a global variable SysBase */
extern struct ExecBase *SysBase;

AROS_UFH2 (void, KPutChar,
	AROS_UFHA(UBYTE, chr, D0),
	AROS_UFHA(struct ExecBase *,SysBase,A3)
)
{
    AROS_USERFUNC_INIT
    RawPutChar(chr);
    AROS_USERFUNC_EXIT
}

void KPrintF(STRPTR format, ...)
{
    AROS_GET_SYSBASE_OK
    va_list args;

    va_start(args, format);
    RawDoFmt(format,args,(VOID_FUNC)KPutChar,SysBase);
    va_end(args);
}
