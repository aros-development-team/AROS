/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc: Runtime debugging support
*/

#ifndef NO_RUNTIME_DEBUG

#include <aros/config.h>
#include <exec/execbase.h>
#include <exec/rawfmt.h>

#include "exec_debug.h"

#if defined(DEBUG_TIMESTAMP)
#include <string.h>
#include "exec_intern.h"
#else
# if defined(DEBUG_USEATOMIC)
# include <aros/atomic.h>
# include <asm/cpu.h>
extern volatile ULONG   _arosdebuglock;
# endif
#endif

#undef KernelBase

#if defined(DEBUG_TIMESTAMP)
extern int Kernel_12_KrnBug(const char *format, va_list args, APTR kernelBase);
#endif

void VLog(struct ExecBase *SysBase, ULONG flags, const char * const *FlagNames, const char *format, va_list args)
{
    struct ExecVLogData {
        const char *flagName;
#if defined(DEBUG_TIMESTAMP)
        const char *fmt;
#endif
    } vlData = {
#if defined(DEBUG_TIMESTAMP)
        NULL,
#endif
        NULL
    };
#if defined(DEBUG_TIMESTAMP)
    TEXT vlFormat[1024];
#else
# if defined(DEBUG_USEATOMIC)
    if (_arosdebuglock & 1)
    {
        while (bit_test_and_set_long((ULONG*)&_arosdebuglock, 1)) { };
    }
# endif
#endif

    /* Prepend tag (if known) */
    if ((vlData.flagName = GetFlagName(flags, FlagNames)) != NULL)
    {
#if defined(DEBUG_TIMESTAMP)
        vlData.fmt = format;
        RawDoFmt(
            "[EXEC] %s: %s",
            (APTR)&vlData, 
            NULL,
            vlFormat
        );
        format = vlFormat;
#else
        RawDoFmt("[EXEC] %s: ", (APTR)&vlData, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL);
#endif
    }
#if defined(DEBUG_TIMESTAMP)
    else
    {
        vlData.fmt = format;
        RawDoFmt("[EXEC] %s", (APTR)&vlData.fmt, (VOID_FUNC)NULL, vlFormat);
        format = vlFormat;
    }

    int fmttlen = strlen(format);
    if (vlFormat[fmttlen] == 0)
    {
        vlFormat[fmttlen] = '\n';
        vlFormat[fmttlen + 1] = 0;
    }
    Kernel_12_KrnBug(format, args, PrivExecBase(SysBase)->KernelBase);
#else
    /* Output the message and append a newline (in order not to bother about it every time) */
    VNewRawDoFmt(format, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL, args);
    RawPutChar('\n');
# if defined(DEBUG_USEATOMIC)
    if (_arosdebuglock & 1)
    {
        __AROS_ATOMIC_AND_L(_arosdebuglock, ~(1 << 1));
    }
# endif
#endif
}
#endif
