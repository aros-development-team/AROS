/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:23  digulla
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

	__AROS_LH3(LONG, Write,

/*  SYNOPSIS */
	__AROS_LA(BPTR, file,   D1),
	__AROS_LA(APTR, buffer, D2),
	__AROS_LA(LONG, length, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 8, Dos)

/*  FUNCTION
	Write some data to a given file. The request is directly
	given to the filesystem - no buffering is involved. For
	small amounts of data it's probably better to use the
	buffered I/O routines.

    INPUTS
	file   - filehandle
	buffer - pointer to data buffer
	length - number of bytes to write. The filesystem is
		 advised to try to fulfill the request as good
		 as possible.

    RESULT
	The number of bytes actually written, -1 if an error happened.
	IoErr() will give additional information in that case.

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
    iofs->IOFS.io_Command=FSA_WRITE;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Args[0]=(LONG)buffer;
    iofs->io_Args[1]=length;

    /* Send the request. */
    DoIO(&iofs->IOFS);
    
    /* Set error code and return */
    
    if((me->pr_Result2=iofs->io_DosError))
	return -1;
    else
	return iofs->io_Args[1];
    __AROS_FUNC_EXIT
} /* Write */
