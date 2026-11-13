/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.

    Desc: Runtime debugging support
*/

/*
 * You can #define this in order to omit the whole code. Perhaps ROM-based systems
 * will want to do this.
 * However it's not advised to do so because this is a great aid in debugging
 * on user's side.
 */
#ifndef NO_RUNTIME_DEBUG

#include <aros/config.h>
#include <exec/execbase.h>

#include <ctype.h>

#include "exec_debug.h"

#include <proto/exec.h>

void ExecLog(struct ExecBase *SysBase, ULONG flags, const char *format, ...)
{
    va_list ap;

    flags &= SysBase->ex_DebugFlags;
    if (!flags)
        return;

    va_start(ap, format);
    VLog(SysBase, flags, ExecFlagNames, format, ap);
    va_end(ap);
}
#endif
