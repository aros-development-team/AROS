/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Rename a file
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include "dos_intern.h"
#include <string.h>
#include <proto/dos.h>

/* An empty path component is the AmigaDOS parent operation: a leading '/',
 * a '/' right after the volume separator, or a doubled '/'. */
static BOOL has_parent_reference(CONST_STRPTR name)
{
    CONST_STRPTR p;

    if (name[0] == '/')
        return TRUE;
    for (p = name; *p; p++)
    {
        if ((p[0] == ':' || p[0] == '/') && p[1] == '/')
            return TRUE;
    }
    return FALSE;
}

#define RENAME_BUFSIZE 256

/* Append 'name' to the directory path in 'buf' (RENAME_BUFSIZE bytes).
 * Returns buf, or 'fallback' if the result does not fit. */
static CONST_STRPTR rename_join(char *buf, CONST_STRPTR name, CONST_STRPTR fallback)
{
    int namelen = strlen(name);
    int len = strlen(buf);

    if (len + namelen < RENAME_BUFSIZE - 1)
    {
        if (buf[len - 1] != ':')
            buf[len++] = '/';
        CopyMem(name, buf + len, namelen + 1);
        return buf;
    }
    return fallback;
}

/* Make 'name' absolute by prefixing the current directory */
static CONST_STRPTR rename_absolute(char *buf, CONST_STRPTR name, struct Process *me, struct DosLibrary *DOSBase)
{
    if (NameFromLock(me->pr_CurrentDir, buf, RENAME_BUFSIZE - 1))
        return rename_join(buf, name, name);
    return name;
}

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

        AROS_LH2(LONG, Rename,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, oldName, D1),
        AROS_LHA(CONST_STRPTR, newName, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 13, Dos)

/*  FUNCTION
        Renames a given file. The old name and the new name must point to the
        same volume.

    INPUTS
        oldName - Name of the file to rename
        newName - New name of the file to rename

    RESULT
        boolean - DOSTRUE or DOSFALSE. IoErr() provides additional information
        on DOSFALSE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DevProc *olddvp, *newdvp;
    LONG status;
    struct Process *me = (struct Process *)FindTask(NULL);
    char buf1[RENAME_BUFSIZE], vol[32];
    char buf2[RENAME_BUFSIZE];
    int len;
    BSTR bstrNewName, bstrOldName;

    ASSERT_VALID_PROCESS(me);

    /* The canonical names built below cost a Lock and a NameFromLock on
     * each side: an Examine and a Parent packet per directory level, some
     * 27 packets before the one that renames. They are needed when a name
     * holds a parent reference (a/b//c); any other name goes to the handler
     * as the caller wrote it, relative to the lock GetDeviceProc() returns. */
    if (!has_parent_reference(oldName) && !has_parent_reference(newName))
        goto send;

    len = SplitName(oldName, ':', vol, 0, sizeof(vol) - 1);

    if (len > 0)
    {
        /* get real name */
        BPTR lock = Lock(oldName, SHARED_LOCK);
        if (lock)
        {
            if (NameFromLock(lock, buf1, sizeof(buf1) - 1))
                oldName = buf1;
            UnLock(lock);
        }
    }
    else
    {
        /* convert to absolute path */
        oldName = rename_absolute(buf1, oldName, me, DOSBase);
    }

    len = SplitName(newName, ':', vol, 0, sizeof(vol) - 1);

    if (len > 0)
    {
        /* get real name of destination path */
        BPTR lock;
        char *pos;
        const char *pos2;

        strcpy(buf2, newName);
        pos = strrchr(buf2, '/');
        if (!pos)
        {
            pos = buf2 + len;
            *pos = '\0';
        }
        else
            *(++pos) = '\0'; /* Keep slashes in name, to support a/b//c => a/c case */

        lock = Lock(buf2, SHARED_LOCK);
        if (lock)
        {
            if (NameFromLock(lock, buf2, sizeof(buf2) - 1))
            {
                pos2 = newName + (int)(pos - buf2);
                newName = rename_join(buf2, pos2, newName);
            }
            UnLock(lock);
        }
    }
    else
    {
        /* convert to absolute path */
        newName = rename_absolute(buf2, newName, me, DOSBase);
    }

    D(bug("[Dos] rename %s %s\n", oldName, newName));

send:
    /* get device pointers */
    if ((olddvp = GetDeviceProc(oldName, NULL)) == NULL ||
        (newdvp = GetDeviceProc(newName, NULL)) == NULL) {
        FreeDeviceProc(olddvp);
        return DOSFALSE;
    }

    /* make sure they're on the same device
     * XXX this is insufficient, see comments in samedevice.c */
    if (olddvp->dvp_Port != newdvp->dvp_Port) {
        FreeDeviceProc(olddvp);
        FreeDeviceProc(newdvp);
        SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
        return DOSFALSE;
    }

    bstrNewName = C2BSTR(newName);
    bstrOldName = C2BSTR(oldName);
    status = dopacket4(DOSBase, NULL, olddvp->dvp_Port, ACTION_RENAME_OBJECT, olddvp->dvp_Lock, bstrOldName, newdvp->dvp_Lock, bstrNewName);
    FREEC2BSTR(bstrOldName);
    FREEC2BSTR(bstrNewName);

    FreeDeviceProc(olddvp);
    FreeDeviceProc(newdvp);

    return status;

    AROS_LIBFUNC_EXIT
} /* Rename */
