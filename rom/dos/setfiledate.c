/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/12/09 13:53:43  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.3  1996/10/24 15:50:37  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/09/21 14:14:23  digulla
    Hand DOSBase to DoName()

    Revision 1.1  1996/09/11 12:54:47  digulla
    A couple of new DOS functions from M. Fleischer

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

	AROS_LH2(BOOL, SetFileDate,

/*  SYNOPSIS */
	AROS_LHA(STRPTR,             name, D1),
	AROS_LHA(struct DateStamp *, date, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 66, Dos)

/*  FUNCTION
	Change the modification time of a file or directory.

    INPUTS
	name - name of the file
	date - new file time

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
    iofs->IOFS.io_Command=FSA_SET_DATE;
    /* io_Args[0] is the name which is set by DoName(). */
    iofs->io_Args[1]=date->ds_Days;
    iofs->io_Args[2]=date->ds_Minute;
    iofs->io_Args[3]=date->ds_Tick;
    return !DoName(iofs,name,DOSBase);
    AROS_LIBFUNC_EXIT
} /* SetFileDate */
