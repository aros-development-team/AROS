/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Examine a directory.
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH5(BOOL, ExAll,

/*  SYNOPSIS */
	AROS_LHA(BPTR,                  lock,    D1),
	AROS_LHA(struct ExAllData *,    buffer,  D2),
	AROS_LHA(LONG,                  size,    D3),
	AROS_LHA(LONG,                  data,    D4),
	AROS_LHA(struct ExAllControl *, control, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 72, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(lock);

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	 =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	 =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh->fh_Device;
    iofs->IOFS.io_Unit	 =fh->fh_Unit;
    iofs->IOFS.io_Command=FSA_EXAMINE_ALL;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Union.io_EXAMINE_ALL.io_ead = buffer;
    iofs->io_Union.io_EXAMINE_ALL.io_Size = size;
    iofs->io_Union.io_EXAMINE_ALL.io_Mode = data;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    /* Set error code and return */
    SetIoErr(iofs->io_DosError);
    if (iofs->io_DosError != 0)
    {
	control->eac_Entries=0;
	return 0;
    }

    for (size=1; buffer!=NULL; size++)
	buffer = buffer->ed_Next;
    control->eac_Entries = size;

    return 1;
    AROS_LIBFUNC_EXIT
} /* ExAll */
