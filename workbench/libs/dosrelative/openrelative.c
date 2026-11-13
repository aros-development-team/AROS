/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Open a file relative to a directory lock.
*/

#include <proto/exec.h>
#include <dos/stdio.h>
#include "dosrelative_intern.h"

static BPTR dosrel_Open(struct DosRelativeBase *DOSRelativeBase, BPTR lock,
                        CONST_STRPTR name, LONG accessMode, LONG soft_nesting);

/*****************************************************************************

    NAME */
#include <proto/dosrelative.h>

        AROS_LH3(BPTR, OpenRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,       D1),
        AROS_LHA(CONST_STRPTR, name,       D2),
        AROS_LHA(LONG,         accessMode, D3),

/*  LOCATION */
        struct DosRelativeBase *, DOSRelativeBase, 6, DosRelative)

/*  FUNCTION
        Opens a file for read and/or write depending on the accessmode
        given, resolving a relative name against the given directory
        lock instead of the current directory of the calling process.

        The name is handed to the filesystem together with the lock, so
        the file is located and opened in a single filesystem operation.

        If name is absolute (names a volume, device or assign, or begins
        with ':'), or is one of the console aliases ("*", "CONSOLE:"),
        lock is ignored. If lock is NULL, the name is resolved against
        the current directory. In both cases the call behaves exactly
        like dos.library/Open().

    INPUTS
        lock       - directory lock to resolve a relative name against,
                     or NULL for the current directory
        name       - NUL terminated name of the file.
        accessMode - One of MODE_OLDFILE   - open existing file
                            MODE_NEWFILE   - delete old, create new file
                                             exclusive lock
                            MODE_READWRITE - open new one if it doesn't exist

    RESULT
        Handle to the file or 0 if the file couldn't be opened.
        IoErr() gives additional information in that case.

    NOTES
        This call is AROS-specific.

        Soft links are followed as by Open(), up to the same nesting
        limit; a link target without a volume specifier is itself
        resolved relative to lock.

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/Open(), LockRelative(), dos.library/Close()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (name == NULL)
        return BNULL;

    D(bug("[OpenRelative] lock=0x%p '%s':%d\n", BADDR(lock), name, accessMode));

    return dosrel_Open(DOSRelativeBase, lock, name, accessMode, DOSREL_MAX_SOFT_LINK_NESTING);

    AROS_LIBFUNC_EXIT
} /* OpenRelative */

/* Open name, following soft links by calling itself. */
static BPTR dosrel_Open(struct DosRelativeBase *DOSRelativeBase, BPTR lock,
                        CONST_STRPTR name, LONG accessMode, LONG soft_nesting)
{
    struct FileHandle *fh;
    struct MsgPort *port;
    BSTR   bname;
    BPTR   ret;
    LONG   error;
    STRPTR softname;

    /*
     * Nothing to resolve relative to: dos.library already does this.
     * "*" is the only console alias without a ':' in it.
     */
    if (lock == BNULL || dosrel_IsAbsolute(name) || strcmp(name, "*") == 0)
        return Open(name, accessMode);

    if (soft_nesting == 0)
    {
        SetIoErr(ERROR_TOO_MANY_LEVELS);
        return BNULL;
    }

    /*
     * The MODE_* values are the ACTION_FIND* packet types:
     * MODE_READWRITE = ACTION_FINDUPDATE, MODE_OLDFILE = ACTION_FINDINPUT,
     * MODE_NEWFILE = ACTION_FINDOUTPUT. Anything else is refused before
     * it reaches a handler.
     */
    if (accessMode != MODE_OLDFILE && accessMode != MODE_NEWFILE && accessMode != MODE_READWRITE)
    {
        SetIoErr(ERROR_NOT_IMPLEMENTED);
        return BNULL;
    }

    fh = AllocDosObject(DOS_FILEHANDLE, NULL);
    if (fh == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return BNULL;
    }

    bname = dosrel_C2BSTR(DOSRelativeBase, name);
    if (bname == BNULL)
    {
        FreeDosObject(DOS_FILEHANDLE, fh);
        SetIoErr(ERROR_NO_FREE_STORE);
        return BNULL;
    }

    port = dosrel_LockPort(lock);

    /*
     * A handler that serves its clients on a port other than the one
     * we found it on says so by filling in fh_Type itself. Handlers
     * that do not care leave it alone and keep talking on the port we
     * used here.
     */
    fh->fh_Type = NULL;
    if (DoPkt(port, accessMode, (SIPTR)MKBADDR(fh), (SIPTR)lock, (SIPTR)bname, 0, 0) != DOSFALSE)
    {
        dosrel_FreeBSTR(DOSRelativeBase, bname);

        if (!fh->fh_Type)
            fh->fh_Type = port;

        ret = MKBADDR(fh);
        if (IsInteractive(ret))
            SetVBuf(ret, NULL, BUF_LINE, -1);

        return ret;
    }

    dosrel_FreeBSTR(DOSRelativeBase, bname);
    error = IoErr();
    FreeDosObject(DOS_FILEHANDLE, fh);

    if (error != ERROR_IS_SOFT_LINK)
    {
        SetIoErr(error);
        return BNULL;
    }

    /*
     * The link target is a path relative to the same lock (or an
     * absolute one, which the recursion hands to Open()).
     */
    softname = dosrel_ReadSoftlink(DOSRelativeBase, lock, name);
    if (softname == NULL)
        return BNULL;

    ret = dosrel_Open(DOSRelativeBase, lock, softname, accessMode, soft_nesting - 1);
    error = IoErr();
    FreeVec(softname);

    SetIoErr(ret ? 0 : error);
    return ret;
}
