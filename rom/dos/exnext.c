/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, ExNext,

/*  SYNOPSIS */
	AROS_LHA(BPTR                  , lock, D1),
	AROS_LHA(struct FileInfoBlock *, fileInfoBlock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 18, Dos)

/*  FUNCTION
        Examine the next entry in a directory.
        

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
        At the moment it is necessary that the lock passed to this
        function is a directory(!!) that has previously been 
        assigned (i.e. assign env: env).

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
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
    iofs->IOFS.io_Command=FSA_EXAMINE_NEXT;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Union.io_EXAMINE_NEXT.io_fib  = fileInfoBlock;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    /* Set error code and return */
    SetIoErr(iofs->io_DosError);
    if(iofs->io_DosError)
      return DOSFALSE;
    else
      return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* ExNext */
