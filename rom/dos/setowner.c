/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/27 00:36:31  ldp
    Polish

    Revision 1.6  1996/12/09 13:53:44  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.5  1996/10/24 15:50:37  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/09/21 14:14:24  digulla
    Hand DOSBase to DoName()

    Revision 1.3  1996/09/11 13:03:08  digulla
    Wrote functions (M. Fleischer)

    Desc:
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
    iofs->io_Args[1]=owner_info>>16;
    iofs->io_Args[2]=owner_info&0xffff;
    return !DoName(iofs,name,DOSBase);
    AROS_LIBFUNC_EXIT
} /* SetOwner */
