/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(BOOL, IsInteractive,

/*  SYNOPSIS */
	__AROS_LA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 36, Dos)

/*  FUNCTION
	Check if file is bound to an interactive device such as a console
	or shell window.

    INPUTS
	file   - filehandle

    RESULT
	!=0 if the file is interactive, 0 if it is not.

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
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

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
    iofs->IOFS.io_Command=FSA_IS_INTERACTIVE;
    iofs->IOFS.io_Flags  =0;

    /* Send the request. */
    DoIO(&iofs->IOFS);
    
    /* Return */
    if(iofs->io_DosError)
	return 0;
    else
	return iofs->io_Args[0];
    
    __AROS_FUNC_EXIT
} /* IsInteractive */
