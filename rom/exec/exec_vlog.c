/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc: Runtime debugging support
*/

#ifndef NO_RUNTIME_DEBUG

#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>

#include "exec_debug.h"

void VLog(struct ExecBase *SysBase, ULONG flags, const char * const *FlagNames, const char *format, va_list args)
{
	const char *flagName;

    /* Prepend tag (if known) */
    if ((flagName = GetFlagName(flags, FlagNames)) != NULL)
    {
        RawDoFmt("[EXEC] %s: ", (APTR)&flagName, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL);
    }

    /* Output the message and append a newline (in order not to bother about it every time) */
    VNewRawDoFmt(format, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL, args);
    RawPutChar('\n');
}

#endif