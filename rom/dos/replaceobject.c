/*
    Copyright (C) 2026 John KNIPPER <code@jkn.me>.
    Distributed under the AROS Public License, version 1.1.

    Desc: Atomically replace a regular file on one filesystem.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, ReplaceObject,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, source, D1),
        AROS_LHA(CONST_STRPTR, target, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 227, Dos)

/*  FUNCTION
        Gives the regular file 'source' the name 'target', replacing a
        regular file that already has that name. The handler performs the
        replacement as one operation: no other program observes 'target'
        missing, and on failure both names are left as they were.

        This is the last step of the usual safe update of a file: write the
        new contents to a temporary file next to the target, close it, then
        replace the target with it.

    INPUTS
        source - Name of the regular file that takes the new name.
        target - Name to give it. If no object has that name, the call is
                 a Rename(). Both names must be on the same volume.

    RESULT
        DOSTRUE on success. On failure DOSFALSE, and IoErr() gives the
        reason, for example:

        ERROR_RENAME_ACROSS_DEVICES - the names are on different volumes.
        ERROR_OBJECT_WRONG_TYPE - source or target is not a regular file.
        ERROR_OBJECT_IN_USE - target is locked or open.
        ERROR_DELETE_PROTECTED - target is protected from deletion.
        ERROR_ACTION_NOT_KNOWN - the handler does not support replacement.

    NOTES
        Handlers implement this with ACTION_REPLACE_OBJECT, whose arguments
        are those of ACTION_RENAME_OBJECT. A caller that must also work with
        handlers answering ERROR_ACTION_NOT_KNOWN can fall back to
        DeleteFile() and Rename(), which are not atomic.

        Naming the same object twice succeeds and changes nothing.

    EXAMPLE

    BUGS

    SEE ALSO
        Rename(), DeleteFile()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DevProc *from, *to;
    BSTR source_name, target_name;
    LONG result, error;

    D(bug("[ReplaceObject] '%s' -> '%s'\n", source, target));

    from = GetDeviceProc(source, NULL);
    if (from == NULL)
        return DOSFALSE;
    to = GetDeviceProc(target, NULL);
    if (to == NULL)
    {
        error = IoErr();
        FreeDeviceProc(from);
        SetIoErr(error);
        return DOSFALSE;
    }

    /* Both names must reach the same handler (see Rename()) */
    if (from->dvp_Port != to->dvp_Port)
    {
        FreeDeviceProc(to);
        FreeDeviceProc(from);
        SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
        return DOSFALSE;
    }

    source_name = (BSTR)C2BSTR(source);
    target_name = (BSTR)C2BSTR(target);
    if (!source_name || !target_name)
    {
        if (target_name)
            FREEC2BSTR(target_name);
        if (source_name)
            FREEC2BSTR(source_name);
        FreeDeviceProc(to);
        FreeDeviceProc(from);
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }

    result = dopacket4(DOSBase, NULL, from->dvp_Port, ACTION_REPLACE_OBJECT,
        from->dvp_Lock, source_name, to->dvp_Lock, target_name);
    error = IoErr();

    FREEC2BSTR(target_name);
    FREEC2BSTR(source_name);
    FreeDeviceProc(to);
    FreeDeviceProc(from);
    SetIoErr(error);

    return result;

    AROS_LIBFUNC_EXIT
} /* ReplaceObject */
