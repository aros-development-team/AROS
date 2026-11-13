/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/console.h>
#include <proto/exec.h>

#include "console_gcc.h"

/*****************************************************************************

    NAME */
        AROS_LH1(void, RemConSnipHook,

/*  SYNOPSIS */
    AROS_LHA(struct Hook *, hook, A0),

/*  LOCATION */
    struct ConsoleBase *, ConsoleDevice, 12, Console)
/*  FUNCTION
        Unlink a hook previously registered by AddConSnipHook(). Removal
        is serialized with selection notification by the console's
        copy-buffer semaphore.

    INPUTS
        hook - Non-NULL pointer to a hook currently registered with
               AddConSnipHook(). Do not remove a hook twice or pass an
               unregistered hook.

    RESULT
        None.

    NOTES
        The hook is unlinked, not freed. When this function returns to a
        caller outside a notification callback, an ongoing notification
        holding the semaphore has finished and the hook can be released.
        Do not call this function from a selection notification callback:
        the console is traversing the same list.

        This function takes the copy-buffer semaphore and may block. The caller must coordinate
        any concurrent registration or removal of the same hook.

    SEE ALSO
        AddConSnipHook()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&ConsoleDevice->copyBufferLock);
    Remove((struct Node *)hook);
    ReleaseSemaphore(&ConsoleDevice->copyBufferLock);

    AROS_LIBFUNC_EXIT
}
