/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>

/*****************************************************************************

    NAME */

      AROS_LH1(struct Process *, FindCLI,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, tasknum, d0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 70, Arp)

/*  NAME
          FindCLI - Get process struct for CLI number

    FUNCTION
	  This function	takes a	CLI task number	(such as given to
	  programs like	Status and Break) and returns a	pointer	to the
	  process structure base, or ZERO if there is no CLI
	  corresponding	to that	number.	 If called with	the special
	  CLI number ZERO, the maximum possible	number of CLI
	  processes is returned.

    INPUTS
	  tasknum -The CLI task	number for the process,	or special
		  case ZERO, which returns the total number of process
		  slots.

    RESULT
	  Process - A process structure, or NULL.  If called with
	       tasknum = 0, the	total number of	allowable CLI
	       processes is returned.

    ADDITIONAL	CONSIDERATIONS
	  This function	does not Forbid() or do	any locking on its
	  own.	You must call this function only while your own	code
	  is running between Forbid() Permit() pairs.

    BUGS
	  None known.
 
    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    if (tasknum == 0)
    {
        #warning FindCLI(0) should return total number of allowable CLI processes.
        return (struct Process *)MaxCli();
    } else {
        return FindCliProc(tasknum);
    }
    
    AROS_LIBFUNC_EXIT
} /* CompareLock */
