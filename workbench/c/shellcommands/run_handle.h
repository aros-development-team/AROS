/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#ifndef SHELL_RUN_HANDLE_H
#define SHELL_RUN_HANDLE_H
/* Run needs its own handle: the parent Shell closes redirections on return.
 * MODE_NEWFILE may hold an exclusive lock, which cannot be duplicated until
 * explicitly shared. Preserve a seekable stream's position (notably >>). */
static BPTR DuplicateRunHandle(BPTR source, struct DosLibrary *DOSBase)
{
    BPTR lock, copy;
    LONG position = -1, error;

    if (!source)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return BNULL;
    }
    if (!IsInteractive(source))
    {
        position = Seek(source, 0, OFFSET_CURRENT);
        if (position < 0)
        {
            error = IoErr();
            /* Only an explicitly unsupported seek permits positionless
             * duplication. Never turn an I/O error into an append at zero.
             * This LONG-based path cannot preserve positions above LONG_MAX. */
            if (position != -1 || error != ERROR_ACTION_NOT_KNOWN)
            {
                SetIoErr(position == -1 && error ? error : ERROR_SEEK_ERROR);
                return BNULL;
            }
        }
    }

    lock = DupLockFromFH(source);
    if (!lock && IoErr() == ERROR_OBJECT_IN_USE)
    {
        if (!ChangeMode(CHANGE_FH, source, SHARED_LOCK))
            return BNULL;
        lock = DupLockFromFH(source);
    }
    if (!lock)
        return BNULL;

    copy = OpenFromLock(lock);
    if (!copy)
    {
        error = IoErr();
        UnLock(lock); /* OpenFromLock consumes the lock only on success. */
        SetIoErr(error);
        return BNULL;
    }
    if (position >= 0 && Seek(copy, position, OFFSET_BEGINNING) == -1)
    {
        error = IoErr();
        Close(copy);
        SetIoErr(error);
        return BNULL;
    }
    return copy;
}
#endif
