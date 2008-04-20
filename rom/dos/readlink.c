/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read the soft link information.
    Lang: English
*/
#include "dos_intern.h"
#include <dos/filesystem.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH5(LONG, ReadLink,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, D1),
	AROS_LHA(BPTR            , lock, D2),
	AROS_LHA(CONST_STRPTR    , path, D3),
	AROS_LHA(STRPTR          , buffer, D4),
	AROS_LHA(ULONG           , size, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 73, Dos)

/*  FUNCTION
	Read the filename referred to by the soft-linked object contained
	in |path| (relative to the lock |lock|) into the buffer |buffer|.
	The variable |path| should contain the name of the object that
	caused the original OBJECT_IS_SOFT_LINK error.

    INPUTS
	port		- The handler to send the request to.
	lock		- Object that |path| is relative to.
	path		- Name of the object that caused the error.
	buffer		- Buffer to fill with resolved filename.
	size		- Length of the buffer.

    RESULT
	!= 0	success
	== 0	failure, see IoErr() for more information.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MakeLink()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IOFileSys iofs;
    struct FileHandle *fh = BADDR(lock);	
    LONG err;

    InitIOFS(&iofs, FSA_READ_SOFTLINK, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;
    iofs.io_Union.io_READ_SOFTLINK.io_Filename = path;
    iofs.io_Union.io_READ_SOFTLINK.io_Buffer   = buffer;
    iofs.io_Union.io_READ_SOFTLINK.io_Size     = size;

    DosDoIO(&iofs.IOFS);
    err = iofs.io_DosError;
    SetIoErr(err);

    return err == 0 ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* ReadLink */
