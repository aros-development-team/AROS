/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  2000/02/17 20:23:13  bergers
    Two functions that work on the root node.

    Revision 1.8  1998/10/20 16:44:46  hkiel
    Amiga Research OS

    Revision 1.7  1997/11/10 17:37:46  turrican
    Return value was off by 1

    Revision 1.6  1997/01/27 00:36:26  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:34  aros
    Added empty templates for all missing functions

    Moved #include's into first column

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
#include <proto/exec.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

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
    
   ULONG * taskarray = (ULONG *)BADDR(DOSBase->dl_Root->rn_TaskArray);
    /* 
       The first ULONG in the taskarray contains the size of the
       taskarray = the max. number of processes the taskarray
       can currently hold. 
    */
    ULONG retval = taskarray[0];
    
    /* 
       Not all of the fields in the array may contain a valid
       pointer to a process and they might be NULL instead. So
       I search that array backwards until I find a valid endtry.
    */
    while (retval && NULL != taskarray[retval])
      retval--;
    
    return retval;
    AROS_LIBFUNC_EXIT
} /* MaxCli */
