/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Write data to a file.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, Write,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,   D1),
	AROS_LHA(APTR, buffer, D2),
	AROS_LHA(LONG, length, D3),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    ASSERT_VALID_PTR(fh);
    ASSERT_VALID_PTR(fh->fh_Device);
    ASSERT_VALID_PTR(fh->fh_Unit);
    ASSERT_VALID_PTR(buffer);

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	 =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	 =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh->fh_Device;
    iofs->IOFS.io_Unit	 =fh->fh_Unit;
    iofs->IOFS.io_Command=FSA_WRITE;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Union.io_WRITE.io_Buffer=buffer;
    iofs->io_Union.io_WRITE.io_Length=length;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    /* Set error code and return */

    if((me->pr_Result2=iofs->io_DosError))
	return -1;
    else
	return iofs->io_Union.io_WRITE.io_Length;
    AROS_LIBFUNC_EXIT
} /* Write */
