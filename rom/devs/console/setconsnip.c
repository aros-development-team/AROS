/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/console.h>

#include <string.h>

#include "console_gcc.h"

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, SetConSnip,

/*  SYNOPSIS */
    AROS_LHA(APTR, data, A0),

/*  LOCATION */
    struct ConsoleBase *, ConsoleDevice, 10, Console)
/*  FUNCTION
        Replace the console selection buffer with a copy of a
        NUL-terminated string. The previous buffer is discarded before
        storage for the replacement is allocated. A NULL pointer or an
        empty string clears the selection buffer.

    INPUTS
        data - Pointer to a readable NUL-terminated string, or NULL to
               clear the buffer. The input remains owned by the caller
               and need only remain valid until this call returns.

    RESULT
        1 if a nonempty string was successfully copied.
        0 if data is NULL, the string is empty, or allocation failed.
        In each of these cases the selection buffer is left empty; the
        previous contents are not restored on failure.

    NOTES
        SetConSnip() does not notify registered hooks. Only bytes before
        the first NUL in data are copied.

        This function takes the copy-buffer semaphore and may block.

    SEE ALSO
        GetConSnip(), AddConSnipHook()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* data = NUL-terminated string */
    ULONG size;
    LONG ret = 0;

    ObtainSemaphore(&ConsoleDevice->copyBufferLock);

    FreeMem((APTR) ConsoleDevice->copyBuffer,
        ConsoleDevice->copyBufferSize);
    ConsoleDevice->copyBuffer = NULL;
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
