/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Rename a file
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

	AROS_LH2(LONG, Rename,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, oldName, D1),
	AROS_LHA(STRPTR, newName, D2),

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
	Calls the action FSA_RENAME on the filesystem-handler.

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    LONG error;
    struct Device *olddev, *newdev;
    struct Process *me=(struct Process *)FindTask(NULL);
    struct IOFileSys io;

    error = DevName(oldName, &olddev, DOSBase);
    if(error)
    {
	SetIoErr(error);
	return DOSFALSE;
    }

    error = DevName(newName, &newdev, DOSBase);
    if(error)
    {
	SetIoErr(error);
	return DOSFALSE;
    }

    if(olddev != newdev)
    {
	SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
	return DOSFALSE;
    }

    /* Prepare I/O request. */
    io.IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    io.IOFS.io_Message.mn_ReplyPort = &me->pr_MsgPort;
    io.IOFS.io_Message.mn_Length = sizeof(struct IOFileSys);
    io.IOFS.io_Flags = 0;
    io.IOFS.io_Command = FSA_RENAME;
    io.io_Union.io_RENAME.io_Filename = oldName;
    io.io_Union.io_RENAME.io_NewName = newName;
    DoName(&io,oldName,DOSBase);

    SetIoErr(io.io_DosError);
    if(io.io_DosError)
    {
	return DOSFALSE;
    }
    else
    {
	return DOSTRUE;
    }

    AROS_LIBFUNC_EXIT
} /* Rename */
