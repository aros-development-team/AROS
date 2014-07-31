/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/console.h>

#include "console_gcc.h"

AROS_LH1(LONG, SetConSnip,
    AROS_LHA(APTR, data, A0),
    struct ConsoleBase *, ConsoleDevice, 10, Console)
{
    AROS_LIBFUNC_INIT

    /* data = NUL-terminated string */
    ULONG size;
    LONG ret = 0;

    ObtainSemaphore(&ConsoleDevice->copyBufferLock);

    FreeMem((APTR) ConsoleDevice->copyBuffer,
        ConsoleDevice->copyBufferSize);
    ConsoleDevice->copyBufferSize = 0;
    if (data)
    {
        size = strlen(data);
        if (size)
        {
            ConsoleDevice->copyBuffer = AllocMem(size, MEMF_PUBLIC);
            if (ConsoleDevice->copyBuffer)
            {
                CopyMem(data, (APTR) ConsoleDevice->copyBuffer, size);
                ConsoleDevice->copyBufferSize = size;
                ret = 1;
            }
        }
    }

    ReleaseSemaphore(&ConsoleDevice->copyBufferLock);

    return ret;

    AROS_LIBFUNC_EXIT
}
