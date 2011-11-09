/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Early bootup section
    Lang: english
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <string.h>

#include "exec_intern.h"

extern void AROS_SLIB_ENTRY(MemoryRawIOInit, Exec, 84)();
extern void AROS_SLIB_ENTRY(MemoryRawPutChar, Exec, 86)(UBYTE chr);

int exec_boot(struct ExecBase *SysBase)
{
    struct TagItem *msg = KrnGetBootInfo();
    char *cmdline = (char *)LibGetTagData(KRN_CmdLine, 0, msg);

    /*
     * Enable type of debug output chosen by user.
     * 'serial' is handled in libbootconsole.
     */
    if (strstr(cmdline, "debug=memory"))
    {
        SetFunction(&SysBase->LibNode, -84 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawIOInit, Exec, 84));
        SetFunction(&SysBase->LibNode, -86 * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawPutChar, Exec, 86));
    }
    RawIOInit();

    return 1;
}

ADD2INITLIB(exec_boot, 0)
