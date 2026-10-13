/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Give the boot stack to the boot task so it is freed with it.
*/

#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "exec_intern.h"

/*
 * The boot code allocates the stack the bootstrap task runs on, so exec
 * leaves it out of the task's tc_MemEntry. Once dos.library has booted,
 * that task calls RemTask(NULL) and the stack would stay allocated forever.
 */
static int Exec_init_bootstack(struct ExecBase *SysBase)
{
    struct Exec_PlatformData *pd = &PrivExecBase(SysBase)->PlatformData;
    struct MemList *ml;

    if (!pd->BootStack)
        return TRUE;

    ml = AllocMem(sizeof(struct MemList), MEMF_PUBLIC | MEMF_CLEAR);
    if (ml)
    {
        ml->ml_NumEntries      = 1;
        ml->ml_ME[0].me_Addr   = pd->BootStack;
        ml->ml_ME[0].me_Length = pd->BootStackSize;
        AddTail(&SysBase->ThisTask->tc_MemEntry, &ml->ml_Node);
    }
    pd->BootStack = NULL;

    return TRUE;
}

ADD2INITLIB(Exec_init_bootstack, 0)
