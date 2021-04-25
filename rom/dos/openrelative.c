/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Open a file with the specified mode.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "dos_intern.h"

static LONG InternalOpenRelative(
    BPTR baselock,
    CONST_STRPTR name,
    LONG accessMode,
    struct FileHandle *handle,
    LONG soft_nesting,
    struct DosLibrary *DOSBase);

#define MAX_SOFT_LINK_NESTING 16 /* Maximum level of soft links nesting */

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(BPTR, OpenRelative,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         lock,       D1),
        AROS_LHA(CONST_STRPTR, name,       D2),
        AROS_LHA(LONG,         accessMode, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 228, Dos)

/*  FUNCTION
        Opens a file for read and/or write depending on the accessmode given.

    INPUTS
        lock       - existing lock to compute filename from
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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *ret;
    LONG error;

    /* Sanity check */
    if (name == NULL)
        return BNULL;

    ASSERT_VALID_PTR(name);

    D(bug("[OpenRelative] baselock=0x%p name=%s mode=%d\n", lock, name, accessMode));

    ret = (struct FileHandle *) AllocDosObject(DOS_FILEHANDLE, NULL);
    if (ret == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return BNULL;
    }

    if (InternalOpenRelative(lock, name, accessMode, ret, MAX_SOFT_LINK_NESTING, DOSBase))
    {
        D(bug("[OpenRelative] returned 0x%p\n", MKBADDR(ret)));
        return MKBADDR(ret);
    }

    D(bug("[OpenRelative] failed, err=%d\n", IoErr()));

    FreeDosObject(DOS_FILEHANDLE, ret);
    return BNULL;

    AROS_LIBFUNC_EXIT
} /* OpenRelative */


static LONG dupHandle(struct FileHandle *fh, BPTR lock, struct DosLibrary *DOSBase)
{
    LONG err;
    struct MsgPort *port;
    struct FileLock *fl;

    if (lock == BNULL)
        return DOSFALSE;

    fl = BADDR(lock);
    port = fl->fl_Task;

    err = dopacket2(DOSBase, NULL, port, ACTION_FH_FROM_LOCK, MKBADDR(fh), lock);

    if (err != DOSFALSE) {
        fh->fh_Type = port;
        if (fh->fh_Interactive)
            SetVBuf(MKBADDR(fh), NULL, BUF_LINE, -1);
        else
            SetVBuf(MKBADDR(fh), NULL, BUF_NONE, -1);
    }

    return err;
}


/* Try to open name recursively calling itself in case it's a soft link.
   Store result in handle. Return boolean value indicating result. */
static LONG InternalOpenRelative(
    BPTR baselock,
    CONST_STRPTR name,
    LONG accessMode,
    struct FileHandle *handle,
    LONG soft_nesting,
    struct DosLibrary *DOSBase)
{
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    LONG ret = DOSFALSE;
    LONG error = 0;
    BPTR con, ast;

    ASSERT_VALID_PROCESS(me);

    D(bug("[OpenRelative] %s: 0x%p \"%s\", Name: \"%s\" File: %p\n",
          __is_process(me) ? "Process" : "Task", me, me->pr_Task.tc_Node.ln_Name,
          name, MKBADDR(handle)));

    if(soft_nesting == 0)
    {
        SetIoErr(ERROR_TOO_MANY_LEVELS);
        return DOSFALSE;
    }

    /* IN:, OUT:, ERR: pseudodevices
     */
    if (pseudoLock(name, (accessMode == MODE_OLDFILE) ? ACCESS_READ :
                         ((accessMode == MODE_NEWFILE) ? ACCESS_WRITE : 0),
                         &ast, &ret, DOSBase)) {
        return dupHandle(handle, ast, DOSBase);
    }

    switch(accessMode)
    {
        case MODE_NEWFILE:
        case MODE_READWRITE:
            con = me->pr_COS;
            ast = me->pr_CES ? me->pr_CES : me->pr_COS;

            break;

        case MODE_OLDFILE:
            ast = con = me->pr_CIS;
            break;

        default:
            SetIoErr(ERROR_NOT_IMPLEMENTED);
            return DOSFALSE;
    }

    if (!Stricmp(name, "CONSOLE:"))
        error = fs_Open(handle, me->pr_ConsoleTask, con, accessMode, name, DOSBase);
    else if (!Stricmp(name, "*"))
        error = fs_Open(handle, me->pr_ConsoleTask, ast, accessMode, name, DOSBase);
    else if (!Stricmp(name, "NIL:"))
    {
        error = fs_Open(handle, BNULL, BNULL, accessMode, name, DOSBase);
        SetIoErr(0);
    }
    else
    {
        BPTR lock = BNULL;
        struct DevProc *dvp = NULL;
        STRPTR filename = strchr(name, ':');

        if (!filename)
        {
            /* No ':', pathname relative to current dir */
            struct MsgPort *port;

            if (baselock) {
                /* baselock provided; relative to it */
                port = ((struct FileLock *)BADDR(baselock))->fl_Task;
                lock = baselock;
            }
            else {
                /* no lock provided; relative to current directory */
                lock = me->pr_CurrentDir;
                if (lock && lock != (BPTR)-1) {
                    port = ((struct FileLock *)BADDR(lock))->fl_Task;
                }
                else {
                    /* no current directory? go from the root */
                    port = DOSBase->dl_Root->rn_BootProc;
                    lock = BNULL;
                }
            }

            error = fs_Open(handle, port, lock, accessMode, name, DOSBase);
        }
        else
        {
            /* absolute path; get the device and look it up against that */
            filename++;
            do
            {
                if ((dvp = GetDeviceProc(name, dvp)) == NULL)
                {
                    error = IoErr();
                    break;
                }

                error = fs_Open(handle, dvp->dvp_Port, dvp->dvp_Lock, accessMode, filename, DOSBase);

            } while(error == ERROR_OBJECT_NOT_FOUND && accessMode != MODE_NEWFILE);

            if (error == ERROR_NO_MORE_ENTRIES)
                error = ERROR_OBJECT_NOT_FOUND;
        }

        if (error == ERROR_IS_SOFT_LINK)
        {
            STRPTR softname = ResolveSoftlink(lock, dvp, name, DOSBase);

            if (softname)
            {
                if (dvp)
                {
                    /*
                     * ResolveSoftlink() gives us path relative to either 'lock lock
                     * (if on current volume), or 'dvp' volume root (if on different volume)
                     * In the latter case we need to do the lookup against the volume root lock.
                     */
                    lock = dvp->dvp_Lock;
                }

                ret = InternalOpenRelative(lock, softname, accessMode, handle, soft_nesting - 1, DOSBase);
                if (ret) {
                    error = 0;
                }
                else {
                    error = IoErr();
                    D(bug("[OpenRelative] Resolve error %d\n", error));
                }

                FreeVec(softname);
            }
            else {
                error = IoErr();
            }
        }

        FreeDeviceProc(dvp);
    }

    if (error)
    {
        SetIoErr(error);
        return DOSFALSE;
    }

    if (IsInteractive(MKBADDR(handle)))
        SetVBuf(MKBADDR(handle), NULL, BUF_LINE, -1);

    return DOSTRUE;
}
