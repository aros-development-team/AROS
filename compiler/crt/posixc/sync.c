/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

/****************************************************************************
    NAME
        void sync(

    SYNOPSIS
        )

    FUNCTION
        Return without performing any synchronization. This
        function is not implemented.

    RESULT
        No return value. errno is unchanged.

    NOTES
        Include <unistd.h>. The call issues no filesystem or
        device requests and does not wait for pending writes.

    BUGS
        The call does not flush buffered data or metadata. Its
        return provides no guarantee that changes have reached
        storage, and no error or diagnostic reports this limitation.

*****************************************************************************/
void sync()
{
    /* TODO: Implement sync() */
}
