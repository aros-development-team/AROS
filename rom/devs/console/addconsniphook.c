/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/console.h>
#include <proto/exec.h>

#include "console_gcc.h"

/*****************************************************************************

    NAME */
        AROS_LH1(void, AddConSnipHook,

/*  SYNOPSIS */
    AROS_LHA(struct Hook *, hook, A0),

/*  LOCATION */
    struct ConsoleBase *, ConsoleDevice, 11, Console)
/*  FUNCTION
        Append a hook to the console selection notification list. The
        console calls the hooks, in list order, when copying a nonempty
        selection and successfully allocating a notification buffer.

    INPUTS
        hook - Non-NULL pointer to an initialized struct Hook with a valid
               entry point. Its list node must not already belong to any
               list. The hook and its associated code and data must remain
               valid until it has been removed with RemConSnipHook().

    RESULT
        None.

    NOTES
        The current implementation passes NULL as the hook object and a
        pointer to a private ConClipData message with this layout:

            struct ConClipData
            {
                ULONG flags;
                ULONG size;
                APTR buffer;
            };

        The flags field is zero, size is the selection length in bytes,
        and buffer points to
        a NUL-terminated copy with size bytes before the terminator.
        The hook return value is ignored. The message and buffer are
        borrowed and valid only during the callback. Do not retain or
        free either pointer; copy data needed after the callback returns.

        Callbacks run while the console holds its copy-buffer semaphore.
        Do not modify the hook list from a callback or wait for another
        task that needs this semaphore. SetConSnip() does not generate
        these notifications. Empty selections and allocation failures do
        not generate them either.

        The caller retains ownership of the hook. Registration takes the
        copy-buffer semaphore and may block.

    SEE ALSO
        RemConSnipHook(), GetConSnip(), SetConSnip()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&ConsoleDevice->copyBufferLock);
    AddTail((struct List *)&ConsoleDevice->sniphooks, (struct Node *)hook);
    ReleaseSemaphore(&ConsoleDevice->copyBufferLock);

    AROS_LIBFUNC_EXIT
}
