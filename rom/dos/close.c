/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:52:45  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.3  1996/08/12 14:17:34  digulla
    Added alias UnLock Close

    Revision 1.2  1996/08/01 17:40:48  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(BOOL, Close,

/*  SYNOPSIS */
	__AROS_LHA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 6, Dos)

/*  FUNCTION
	Close a filehandle opened with Open(). If the file was used
	with buffered I/O the final write may fail and thus Close()
	return an error. The file is closed in any case.

    INPUTS
	file   - filehandle

    RESULT
	0 if there was an error. !=0 on success.

    NOTES
	This function is identical to Lock().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/

/*****************************************************************************

    NAME
	#include <clib/dos_protos.h>

	__AROS_LH1(BOOL, UnLock,

    SYNOPSIS
	__AROS_LHA(BPTR, lock, D1),

    LOCATION
	struct DosLibrary *, DOSBase, 15, Dos)

    FUNCTION
	Free a lock created with Lock().

    INPUTS
	lock - The lock to free

    RESULT

    NOTES
	This function is identical to Close() - see there.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
/*AROS alias UnLock Close */
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    /* The returncode defaults to OK. */
    BOOL ret=1;

    /* 0 handles are OK */
    if(!file)
	return ret;

    /* If the filehandle has a pending write on it Flush() the buffer. */
    if(fh->fh_Flags&FHF_WRITE)
	ret=Flush(file);

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	 =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	 =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh->fh_Device;
    iofs->IOFS.io_Unit	 =fh->fh_Unit;
    iofs->IOFS.io_Command=FSA_CLOSE;
    iofs->IOFS.io_Flags  =0;

    /* Send the request. No errors possible. */
    (void)DoIO(&iofs->IOFS);

    FreeDosObject(DOS_FILEHANDLE,fh);

    /* All done. */
    return ret;
    __AROS_FUNC_EXIT
} /* Close */
