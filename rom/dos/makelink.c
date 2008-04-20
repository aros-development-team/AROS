/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
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
	AROS_LHA(CONST_STRPTR, name, D1),
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

    struct IOFileSys iofs;
    struct DevProc *dvp;
    struct FileHandle *fh;
    LONG err;

    /* soft link is easy */
    if (soft) {
        InitIOFS(&iofs, FSA_CREATE_SOFTLINK, DOSBase);
        iofs.io_Union.io_CREATE_SOFTLINK.io_Reference = (STRPTR) dest;
        return DoIOFS(&iofs, NULL, name, DOSBase) == 0 ? DOSTRUE : DOSFALSE;
    }

    /* hard link. find the handler */
    if ((dvp = GetDeviceProc(name, NULL)) == NULL)
        return DOSFALSE;

    fh = (struct FileHandle *) BADDR(dest);

    /* source and target must be on the same device
     * XXX this is insufficient, see comments in samedevice.c */
    if (dvp->dvp_Port != (struct MsgPort *)fh->fh_Device) {
        FreeDeviceProc(dvp);
        SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
        return DOSFALSE;
    }

    InitIOFS(&iofs, FSA_CREATE_HARDLINK, DOSBase);
    iofs.io_Union.io_CREATE_HARDLINK.io_OldFile = fh->fh_Unit;
    err = DoIOFS(&iofs, dvp, name, DOSBase);

    FreeDeviceProc(dvp);

    return err == 0 ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* MakeLink */
