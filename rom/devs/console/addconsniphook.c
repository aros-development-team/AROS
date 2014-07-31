/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/console.h>
#include <proto/exec.h>

#include "console_gcc.h"

AROS_LH1(void, AddConSnipHook,
    AROS_LHA(struct Hook *, hook, A0),
    struct ConsoleBase *, ConsoleDevice, 11, Console)
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&ConsoleDevice->copyBufferLock);
    AddTail((struct List *)&ConsoleDevice->sniphooks, (struct Node *)hook);
    ReleaseSemaphore(&ConsoleDevice->copyBufferLock);

    AROS_LIBFUNC_EXIT
}
