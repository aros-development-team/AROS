/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/dos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */

      AROS_LH1(struct Process *, FindProcess,

/*  SYNOPSIS */ 
      AROS_LHA(LONG, tasknum, D0),

/*  LOCATION */
      struct ArpBase *, ArpBase, ?, Arp)

/*  NAME
        FindProcess -- Find a process given a CLI task number.

    SYNOPSIS
        Process = FindProcess(tasknum)
           d0			 d0

    FUNCTION
        This function returns the pointer to the process structure
        associated with tasknum.  This is a pointer to the start of
        the process structure, usable with EXEC calls, and not the
        process value returned by DOS calls.

    INPUTS
        The CLI task number for the process, or special case ZERO,
 	      which returns the total number of process slots.

 	      NOTE:  YOU MUST FORBID PRIOR TO CALLING THIS FUNCTION!

        This function only makes sense when you are Forbidden, as
        otherwise the process may squirt away from you before you
        can use the result!  Thus this function does NOT Forbid
        around a critical section.  You have been warned!

    RESULT
        Process -- a pointer to a Process structure, or NULL.  A NULL
                   return indicates that there is no currently active
                   process associated with that CLI task number.

 		( Except if tasknum was Zero )

    BUGS
        None known.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)
  
  if (0 != tasknum)
    return FindCliProc(tasknum);
  else
  {
    /* return total number of process slots */
  }
  return 0;

  AROS_LIBFUNC_EXIT
} /* FindProcess */
