/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/11 12:54:45  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/filesystem.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH5(void, ExAllEnd,

/*  SYNOPSIS */
	__AROS_LHA(BPTR,                  lock,    D1),
	__AROS_LHA(struct ExAllData *,    buffer,  D2),
	__AROS_LHA(LONG,                  size,    D3),
	__AROS_LHA(LONG,                  data,    D4),
	__AROS_LHA(struct ExAllControl *, control, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 165, Dos)

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(lock);

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh->fh_Device;
    iofs->IOFS.io_Unit   =fh->fh_Unit;
    iofs->IOFS.io_Command=FSA_EXAMINE_ALL_END;
    iofs->IOFS.io_Flags  =0;

    /* Send the request. May not fail. */
    DoIO(&iofs->IOFS);
    __AROS_FUNC_EXIT
} /* ExAllEnd */
