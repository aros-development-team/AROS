/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Early bootup section
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include <defines/exec_LVO.h>

#include <string.h>

#include "exec_intern.h"

extern void AROS_SLIB_ENTRY(MemoryRawIOInit, Exec, LVORawIOInit)();
extern void AROS_SLIB_ENTRY(MemoryRawPutChar, Exec, LVORawPutChar)(UBYTE chr);

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
        SetFunction(&SysBase->LibNode, -LVORawIOInit * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawIOInit, Exec, LVORawIOInit));
        SetFunction(&SysBase->LibNode, -LVORawPutChar * LIB_VECTSIZE,
            AROS_SLIB_ENTRY(MemoryRawPutChar, Exec, LVORawPutChar));
    }
    RawIOInit();

    return 1;
}

ADD2INITLIB(exec_boot, 0)
