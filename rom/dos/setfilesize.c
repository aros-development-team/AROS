/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/11 12:54:47  digulla
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

	__AROS_LH3(LONG, SetFileSize,

/*  SYNOPSIS */
	__AROS_LHA(BPTR, file,   D1),
	__AROS_LHA(LONG, offset, D2),
	__AROS_LHA(LONG, mode,   D3),

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
    iofs->IOFS.io_Command=FSA_READ;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Args[0]=offset<0?-1:0;
    iofs->io_Args[1]=offset;
    iofs->io_Args[2]=mode;

    /* Send the request. */
    DoIO(&iofs->IOFS);
    
    /* Set error code and return */
    if((me->pr_Result2=iofs->io_DosError))
        return -1;    
    else
        return iofs->io_Args[1];
    
    __AROS_FUNC_EXIT
} /* SetFileSize */
