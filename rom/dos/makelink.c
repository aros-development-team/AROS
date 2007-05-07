/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a hard- or softlink.
    Lang: English
*/
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

	AROS_LH3(LONG, MakeLink,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(APTR,   dest, D2),
	AROS_LHA(LONG  , soft, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 74, Dos)

/*  FUNCTION
        MakeLink() will create a link between two files or directories.
        A link is a filesystem object that refers to another file.

        A soft link refers to another file by name, and is resolved by
        the filesystem and the caller. Soft links are not restricted to
        the same volume. The |dest| argument is a NUL terminated pathname
        to the pre-existing object. Soft links can be used on directories.

        A hard link refers to another file by the location on a disk, and
        is resolved by the filesystem. Hard links are restricted to files
        on the same volume. The |dest| argument is a lock on another file.

    INPUTS
	name - The name of the link to create
	dest - If 'soft' is TRUE this must be a filename, if it is FALSE a BPTR
	       pointing to the file to be hard-linked must be provided
	soft - TRUE, if a soft-link is to be created, FALSE for an hard-link

    RESULT
	boolean - DOSTRUE or DOSFALSE. On error, IoErr() will contain more
	information.

    NOTES

    EXAMPLE

    BUGS
        Soft links were not working in the ROM filesystem before version
        37.

    SEE ALSO
        ReadLink()
 
    INTERNALS
	This function calls either FSA_CREATE_HARDLINK or FSA_CREATE_SOFTLINK
	on the filesystem of `name`.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG error;
    struct Device *dev;
    struct Process *me=(struct Process *)FindTask(NULL);
    struct IOFileSys io;

    io.IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    io.IOFS.io_Message.mn_ReplyPort = &me->pr_MsgPort;
    io.IOFS.io_Message.mn_Length = sizeof(struct IOFileSys);
    io.IOFS.io_Flags = 0;
    if (soft)
    {
        /* We want a soft-link. */
        io.IOFS.io_Command = FSA_CREATE_SOFTLINK;
        io.io_Union.io_CREATE_SOFTLINK.io_Reference = (STRPTR)dest;
    } else
    {
        /* We want a hard-link. */
	struct FileHandle *fh = (struct FileHandle *)BADDR((BPTR)dest);
        /* We check, if name and dest are on the same device. */
        if (DevName(name, &dev, DOSBase))
	    return DOSFALSE;
	if (dev != fh->fh_Device)
	{
	    SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
	    return DOSFALSE;
	}
	io.IOFS.io_Command = FSA_CREATE_HARDLINK;
	io.io_Union.io_CREATE_HARDLINK.io_OldFile = fh->fh_Unit;
    }

    error = DoName(&io, name, DOSBase);
    if (error)
    {
        SetIoErr(error);
        return DOSFALSE;
    }

    return DOSTRUE;
    AROS_LIBFUNC_EXIT
} /* MakeLink */
