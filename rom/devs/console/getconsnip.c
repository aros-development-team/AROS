/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/console.h>
#include <proto/exec.h>

#include "console_gcc.h"

AROS_LH0(APTR, GetConSnip, struct ConsoleBase *, ConsoleDevice, 9, Console)
{
    AROS_LIBFUNC_INIT

    APTR data = NULL;

    ObtainSemaphore(&ConsoleDevice->copyBufferLock);
    if (ConsoleDevice->copyBuffer)
    {
        /* OS2-3.x C:conclip calls FreeVec(). NUL-terminated */
        data = AllocVec(ConsoleDevice->copyBufferSize + 1, MEMF_CLEAR);
        if (data)
            CopyMem(ConsoleDevice->copyBuffer, data,
                ConsoleDevice->copyBufferSize);
    }
    ReleaseSemaphore(&ConsoleDevice->copyBufferLock);

    return data;

    AROS_LIBFUNC_EXIT
}
