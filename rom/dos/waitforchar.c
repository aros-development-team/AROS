/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Waits for a character to arrive at a filehandle.
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>
#include <exec/types.h>

	AROS_LH2(LONG, WaitForChar,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file, D1),
	AROS_LHA(LONG, timeout, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 34, Dos)

/*  FUNCTION

    INPUTS

    RESULT

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

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    /* Null-pointers are okay. */
    if (!file)
        return 0;

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh->fh_Device;
    iofs->IOFS.io_Unit   =fh->fh_Unit;
    iofs->IOFS.io_Command=FSA_WAIT_CHAR;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Union.io_WAIT_CHAR.io_Timeout=timeout;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    /* Set error code */
    if(!iofs->io_DosError)
        return iofs->io_Union.io_WAIT_CHAR.io_Success;
    else
        SetIoErr(iofs->io_DosError);
    return 0;
    AROS_LIBFUNC_EXIT
} /* WaitForChar */
