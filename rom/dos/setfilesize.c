/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Change the size of a file.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, SetFileSize,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,   D1),
	AROS_LHA(LONG, offset, D2),
	AROS_LHA(LONG, mode,   D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 76, Dos)

/*  FUNCTION
	Change the size of a file.

    INPUTS
	file   - filehandle
	offset - relative size
	mode   - OFFSET_BEGINNING, OFFSET_CURRENT or OFFSET_END

    RESULT
	New size of the file or -1 in case of an error.
	IoErr() gives additional information in that case.

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

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh->fh_Device;
    iofs->IOFS.io_Unit   =fh->fh_Unit;
    iofs->IOFS.io_Command=FSA_SET_FILE_SIZE;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Union.io_SET_FILE_SIZE.io_Negative=offset<0?-1:0;
    iofs->io_Union.io_SET_FILE_SIZE.io_Offset  =offset;
    iofs->io_Union.io_SET_FILE_SIZE.io_SeekMode=mode;

    /* Send the request. */
    DoIO(&iofs->IOFS);
    
    /* Set error code and return */
    if((me->pr_Result2=iofs->io_DosError))
        return -1;    
    else
        return iofs->io_Union.io_SET_FILE_SIZE.io_Offset;
    
    AROS_LIBFUNC_EXIT
} /* SetFileSize */
