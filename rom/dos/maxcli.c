/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/10/24 15:50:33  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/10/10 13:21:22  digulla
    Returns ULONG instead of BPTR (Fleischer)

    Revision 1.2  1996/08/01 17:40:55  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	AROS_LH0(ULONG, MaxCli,

/*  SYNOPSIS */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 92, Dos)

/*  FUNCTION
	Returns the highest Cli number currently in use. Since processes
	may be added and removed at any time the returned value may already
	be wrong.

    INPUTS

    RESULT
	Maximum Cli number (_not_ the number of Clis).

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
    
    return DOSBase->dl_ProcCnt-1;
    AROS_LIBFUNC_EXIT
} /* MaxCli */
