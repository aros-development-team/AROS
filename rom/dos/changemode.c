/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/12/09 13:53:21  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.2  1996/10/24 15:50:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/11 12:54:45  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH3(BOOL, ChangeMode,

/*  SYNOPSIS */
	AROS_LHA(ULONG, type,    D1),
	AROS_LHA(BPTR,  object,  D2),
	AROS_LHA(ULONG, newmode, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 75, Dos)

/*  FUNCTION
	Try to change the mode used by a lock or filehandle.

    INPUTS
	type    - CHANGE_FH or CHANGE_LOCK.
	object  - Filehandle or lock.
	newmode - New mode.

    RESULT
	!=0 if all went well, 0 else. IoErr() gives additional information
	in that case.

    NOTES
	Since filehandles and locks are identical under AROS the type
	argument is ignored.

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
    struct FileHandle *fh=(struct FileHandle *)BADDR(object);

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
    iofs->IOFS.io_Command=FSA_FILE_MODE;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Args[0]=newmode;

    /* Send the request. */
    DoIO(&iofs->IOFS);
    
    /* Set error code and return */
    return (me->pr_Result2=iofs->io_DosError)==0;
    AROS_LIBFUNC_EXIT
} /* ChangeMode */
