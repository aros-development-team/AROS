/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: close.c 34705 2010-10-13 20:30:16Z jmcmullan $

    Desc:
    Lang: English
*/
#define DEBUG 1
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, Close,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 6, Dos)

/*  FUNCTION
	Close a filehandle opened with Open(). If the file was used
	with buffered I/O the final write may fail and thus Close()
	return an error. The file is closed in any case.

    INPUTS
	file  --  filehandle

    RESULT
	0 if there was an error. != 0 on success.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);
    /* The returncode defaults to OK. */
    BOOL ret = 1;

    D(bug("Close(%x)\n", fh));
    ASSERT_VALID_PTR_OR_NULL(fh);

    /* 0 handles are OK */
    if(file == BNULL)
	return ret;

    /* If the filehandle has a pending write on it Flush() the buffer. */
    if(fh->fh_Flags & FHF_WRITE)
	ret = Flush(file);

    ret = dopacket1(DOSBase, NULL, fh->fh_Type, ACTION_END, fh->fh_Arg1);

    /* Free the filehandle which was allocated in Open(), CreateDir()
       and such. */
    FreeDosObject(DOS_FILEHANDLE, fh);

    return ret;

    AROS_LIBFUNC_EXIT
} /* Close */
