/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Delete a file or directory.
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, DeleteFile,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 12, Dos)

/*  FUNCTION
	Tries to delete a file or directory by a given name.
	May fail if the file is in use or protected from deletion.

    INPUTS
	name	   - NUL terminated name.

    RESULT
	!=0 if the file is gone, 0 if is still there.
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

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	 =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	 =sizeof(struct IOFileSys);
    iofs->IOFS.io_Flags=0;
    iofs->IOFS.io_Command=FSA_DELETE_OBJECT;

    DoName(iofs,name,DOSBase);
    SetIoErr(iofs->io_DosError);

    if (iofs->io_DosError)
	return DOSFALSE;
    return DOSTRUE;
    AROS_LIBFUNC_EXIT
} /* DeleteFile */
