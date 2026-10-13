/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/console.h>
#include <proto/exec.h>

#include "console_gcc.h"

/*****************************************************************************

    NAME */
        AROS_LH0(APTR, GetConSnip,

/*  LOCATION */
        struct ConsoleBase *, ConsoleDevice, 9, Console)
/*  FUNCTION
        Obtain a private, NUL-terminated copy of the console selection
        buffer.

    INPUTS
        None.

    RESULT
        A newly allocated copy, or NULL if there is no stored buffer or
        memory could not be allocated. Release a non-NULL result with
        FreeVec(), not FreeMem().

    NOTES
        The returned memory belongs to the caller and remains valid across
        later changes to the console selection. No length is returned.
        This function takes the copy-buffer semaphore and may block.

    SEE ALSO
        SetConSnip(), AddConSnipHook(), exec.library/FreeVec()

*****************************************************************************/
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
