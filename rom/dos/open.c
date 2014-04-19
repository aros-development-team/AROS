/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file with the specified mode.
    Lang: english
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

#include "dos_intern.h"

static LONG InternalOpen(CONST_STRPTR name, LONG accessMode, 
    struct FileHandle *handle, LONG soft_nesting, struct DosLibrary *DOSBase);

#define MAX_SOFT_LINK_NESTING 16 /* Maximum level of soft links nesting */

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BPTR, Open,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name,       D1),
        AROS_LHA(LONG,         accessMode, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 5, Dos)

/*  FUNCTION
        Opens a file for read and/or write depending on the accessmode given.

    INPUTS
        name       - NUL terminated name of the file.
        accessMode - One of MODE_OLDFILE   - open existing file
                            MODE_NEWFILE   - delete old, create new file
                                             exclusive lock
                            MODE_READWRITE - open new one if it doesn't exist

    RESULT
        Handle to the file or 0 if the file couldn't be opened.
        IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *ret;
    LONG error;

    /* Sanity check */
    if (name == NULL) return BNULL;

    /* Create filehandle */
    ret = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL);

    if (ret != NULL)
    {
        LONG ok = InternalOpen(name, accessMode, ret, MAX_SOFT_LINK_NESTING, DOSBase);
        D(bug("[Open] = %p, Error = %d\n", ok ? MKBADDR(ret) : BNULL, IoErr()));
        if (ok)
        {
            return MKBADDR(ret);            
        }
        else
        {
            error = IoErr();
            FreeDosObject(DOS_FILEHANDLE,ret);
        }
    }
    else
        error = ERROR_NO_FREE_STORE;

    SetIoErr(error);
    return BNULL;

    AROS_LIBFUNC_EXIT
} /* Open */


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
static LONG InternalOpen(CONST_STRPTR name, LONG accessMode, 
    struct FileHandle *handle, LONG soft_nesting, struct DosLibrary *DOSBase)
{
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    LONG ret = DOSFALSE;
    LONG error = 0;
    BPTR con, ast;

    ASSERT_VALID_PROCESS(me);

    D(bug("[Open] %s: 0x%p \"%s\", Name: \"%s\" File: %p\n",
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
        BPTR cur = BNULL;
        struct DevProc *dvp = NULL;
        STRPTR filename = strchr(name, ':');

        if (!filename)
        {
            struct MsgPort *port;

            /* No ':', pathname relative to current dir */
            cur = me->pr_CurrentDir;

            if (cur && cur != (BPTR)-1) {
                port = ((struct FileLock *)BADDR(cur))->fl_Task;
            } else {
                port = DOSBase->dl_Root->rn_BootProc;
                cur = BNULL;
            }

            error = fs_Open(handle, port, cur, accessMode, name, DOSBase);
        }
        else 
        {
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
            STRPTR softname = ResolveSoftlink(cur, dvp, name, DOSBase);

            if (softname)
            {
                /* All OK */
                BPTR olddir = BNULL;

                if (dvp)
                {
                    olddir = me->pr_CurrentDir;
                    error = RootDir(dvp, DOSBase);
                }
                else
                    error = 0;

                if (!error)
                {
                    ret = InternalOpen(softname, accessMode, handle, soft_nesting - 1, DOSBase);
                    error = ret ? 0 : IoErr();

                    if (olddir)
                        UnLock(CurrentDir(olddir));
                }

                FreeVec(softname);
            }
            else
                error = IoErr();
        }

        FreeDeviceProc(dvp);
    }

    if (!error)
    {
        if (IsInteractive(MKBADDR(handle)))
            SetVBuf(MKBADDR(handle), NULL, BUF_LINE, -1);
        
        return DOSTRUE;
    }
    else
    {
        SetIoErr(error);
        return DOSFALSE;
    }
}
