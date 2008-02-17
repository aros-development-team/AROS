/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file with the specified mode.
    Lang: english
*/
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

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
	name	   - NUL terminated name of the file.
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
    BPTR con, ast;
    LONG error;
    struct Process *me;

    /* Sanity check */
    if (name == NULL) return NULL;

    /* Get pointer to process structure */
    me = (struct Process *)FindTask(NULL);

    /* check for an empty filename. this supports this form that was required
     * pre-2.0, which didn't have OpenFromLock():
     *
     *     old = CurrentDir(lock);
     *     fh = Open("", MODE_OLDFILE);
     *     CurrentDir(old);
     */
    if (*name == '\0')
	return OpenFromLock(DupLock(me->pr_CurrentDir));
    
    /* Create filehandle */
    ret = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL);

    if(ret != NULL)
    {
        LONG doappend = 0;

	/* Get pointer to I/O request. Use stackspace for now. */
	struct IOFileSys iofs;

	/* Prepare I/O request. */
	InitIOFS(&iofs, FSA_OPEN_FILE, DOSBase);

	switch(accessMode)
	{
	case MODE_OLDFILE:
	    iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_OLDFILE;
	    ast = con = me->pr_CIS;
	    break;

	case MODE_NEWFILE:
	    iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_NEWFILE;
	    con = me->pr_COS;
	    ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    break;

	case MODE_READWRITE:
	    iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_READWRITE;
	    con = me->pr_COS;
	    ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    break;

	default:
	    /* See if the user requested append mode */
	    doappend   = accessMode & FMF_APPEND;

	    /* The append mode is all taken care by dos.library */
	    accessMode &= ~FMF_APPEND;

	    iofs.io_Union.io_OPEN_FILE.io_FileMode = accessMode;
	    ast = con = me->pr_CIS;
	    break;
	}

	iofs.io_Union.io_OPEN_FILE.io_Protection = 0;

	if(!Stricmp(name, "CONSOLE:"))
	{
	    iofs.IOFS.io_Device = ((struct FileHandle *)BADDR(con))->fh_Device;
	    iofs.IOFS.io_Unit   = ((struct FileHandle *)BADDR(con))->fh_Unit;
	    iofs.io_Union.io_OPEN_FILE.io_Filename = "";
	    DosDoIO(&iofs.IOFS);
	    error = me->pr_Result2 = iofs.io_DosError;
	}
	else
	if(!Stricmp(name, "*"))
	{
	    iofs.IOFS.io_Device = ((struct FileHandle *)BADDR(ast))->fh_Device;
	    iofs.IOFS.io_Unit   = ((struct FileHandle *)BADDR(ast))->fh_Unit;
	    iofs.io_Union.io_OPEN_FILE.io_Filename = "";
	    DosDoIO(&iofs.IOFS);
	    error = me->pr_Result2 = iofs.io_DosError;
	}
	else {
            struct DevProc *dvp = NULL;

            iofs.io_Union.io_OPEN_FILE.io_Filename = StripVolume(name);

            do {
                if ((dvp = GetDeviceProc(name, dvp)) == NULL) {
                    error = IoErr();
                    break;
                }

                error = DoIOFS(&iofs, dvp, NULL, DOSBase);
            } while(error == ERROR_OBJECT_NOT_FOUND && accessMode != MODE_NEWFILE);

            if (error == ERROR_NO_MORE_ENTRIES)
                error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;

            FreeDeviceProc(dvp);
        }

	if(error == 0)
	{
	    ret->fh_Device = iofs.IOFS.io_Device;
	    ret->fh_Unit   = iofs.IOFS.io_Unit;
	    if (doappend)
	    {
		 /* See if the handler supports FSA_SEEK */
		 if (Seek(MKBADDR(ret), 0, OFFSET_END) != -1)
		 {
		     /* if so then set the proper flag in the FileHandle struct */
		     ret->fh_Flags |= FHF_APPEND;
		 }
	    }
	    if (IsInteractive(MKBADDR(ret)))
	        SetVBuf(MKBADDR(ret), NULL, BUF_LINE, -1);

	    return MKBADDR(ret);
	}

	FreeDosObject(DOS_FILEHANDLE,ret);
    }

    else
	SetIoErr(ERROR_NO_FREE_STORE);

    return 0;

    AROS_LIBFUNC_EXIT
} /* Open */
