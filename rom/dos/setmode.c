/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Set the current mode of a console device.
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, SetMode,

/*  SYNOPSIS */
	AROS_LHA(BPTR, fh, D1),
	AROS_LHA(LONG, mode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 71, Dos)

/*  FUNCTION
	SetMode() can be used to change a console handler between
	RAW: mode and CON: mode.

    INPUTS
	fh      -   The filehandle describing the console.
	mode    -   The new mode of the console:
			1   - RAW: mode
			0   - CON: mode

    RESULT
	This function will return whether it succeeded:

	== DOSTRUE  console mode changed
	!= DOSTRUE  console mode change failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /*
	Fairly simple, create a packet, and send it to the
	required handler. Lets just hope it understands it.

    */
    struct IOFileSys     iofs;
    struct FileHandle   *fh = (struct FileHandle *)BADDR(file)
    struct Process      *me = (struct Process *)FindTask(NULL);

    iofs.IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    iofs.IOFS.io_Message.mn_ReplyPort = &me->pr_MsgPort;
    iofs.IOFS.io_Message.mn_Length = sizeof(iofs);
    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit = fh->fh_Unit;
    iofs.IOFS.io_Command = FSA_CONSOLE_MODE;
    iofs.IOFS.io_Flags = 0;
    iofs.io_Union.io_CONSOLE_MODE.io_Mode = mode;

    DoIO(&iofs.IOFS);

    return iofs.io_DosError;

    AROS_LIBFUNC_EXIT
} /* SetMode */
