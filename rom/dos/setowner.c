/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Set the owner of a file.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, SetOwner,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name,       D1),
	AROS_LHA(ULONG,  owner_info, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 166, Dos)

/*  FUNCTION

    INPUTS
	name	   - name of the file
	owner_info - (UID<<16)+GID

    RESULT
	!=0 if all went well, 0 else. IoErr() gives additional
	information in that case.

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
    iofs->IOFS.io_Command=FSA_SET_OWNER;
    /* io_Args[0] is the name which is set by DoName(). */
    iofs->io_Union.io_SET_OWNER.io_UID=owner_info>>16;
    iofs->io_Union.io_SET_OWNER.io_GID=owner_info&0xffff;
    return !DoName(iofs,name,DOSBase);
    AROS_LIBFUNC_EXIT
} /* SetOwner */
