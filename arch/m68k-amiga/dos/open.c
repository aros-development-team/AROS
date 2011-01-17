/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file with the specified mode.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>
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


static LONG mungeaccessmode(LONG);

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
    LONG error;

    /* Sanity check */
    if (name == NULL) return BNULL;

    /* Create filehandle */
    ret = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL);

    if(ret != NULL)
    {
    	accessMode = mungeaccessmode(accessMode);
	if(InternalOpen(name, accessMode, ret, MAX_SOFT_LINK_NESTING, DOSBase))
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

static LONG mungeaccessmode(LONG mode)
{
	if (mode == MODE_READWRITE)
		return ACTION_FINDUPDATE;
	if (mode == MODE_NEWFILE)
		return ACTION_FINDOUTPUT;
	if (mode == MODE_OLDFILE)
		return ACTION_FINDINPUT;
	if (mode & FMF_CREATE)
		return ACTION_FINDOUTPUT;
	if (mode & FMF_CLEAR)
		return ACTION_FINDOUTPUT;
	if (mode & (FMF_READ | FMF_WRITE))
		return ACTION_FINDINPUT;
	bug("unknown access mode %x\n", mode);
	for(;;);
}
