/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, Relabel,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, drive, D1),
	AROS_LHA(STRPTR, newname, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 120, Dos)

/*  FUNCTION

    Change names of a volume.

    INPUTS

    drive    --  The name of the device to rename (including the ':').
    newname  --  The new name for the device (without the ':').

    RESULT

    A boolean telling whether the name change was successful or not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    28.04.2000  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    LONG error;
    struct Device  *dev;
    struct Process *me = (struct Process *)FindTask(NULL);
    struct IOFileSys io;

    error = DevName(drive, &dev, DOSBase);
    if(error)
    {
	SetIoErr(error);
	return DOSFALSE;
    }

    /* Prepare I/O request. */
    io.IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    io.IOFS.io_Message.mn_ReplyPort = &me->pr_MsgPort;
    io.IOFS.io_Message.mn_Length = sizeof(struct IOFileSys);
    io.IOFS.io_Device = dev;
    io.IOFS.io_Unit = NULL;	/* No unit needed as all information is in the
				   io_RELABEL struct. */
    io.IOFS.io_Flags = 0;
    io.IOFS.io_Command = FSA_RELABEL;
    io.io_Union.io_RELABEL.io_NewName = newname;
    io.io_Union.io_RELABEL.io_Result  = FALSE;

    error = DoIO(&io.IOFS);

    if(error)
    {
        SetIoErr(error);
	return DOSFALSE;
    }

    return io.io_Union.io_RELABEL.io_Result ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* Relabel */
