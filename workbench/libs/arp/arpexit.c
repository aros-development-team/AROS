/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH2(void, ArpExit,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, ReturnCode, D0),
      AROS_LHA(ULONG, fault     , D2),

/*  LOCATION */
      struct ArpBase *, ArpBase, 63, Arp)

/*  NAME
        ArpExit -- exit immediately, closing arp, freeing resources.
 
    SYNOPSIS
        ArpExit( ReturnCode, (Fault) )
                    d0		d2
 
    FUNCTION
        This function will cause a currently running program to
        terminate.  It will first CloseLibrary(ArpBase), which will
        cause all tracked resources to be freed.  It will then force
        DOS to unload your program, returning the error code to the CLI.
 
    INPUTS
        ReturnCode -- The integer value you wish to return. By
                        convention, 0 means a normal exit.
 	      Fault -- If ReturnCode is non-zero, this value is the ADOS
 			           error code which is used with the "Why" program,
 			           in pr_Result2. ( If ReturnCode is ZERO, pr_Result2
 			           will be set to 0).
 
    RESULT
        No results, in fact, no return.
 
    BUGS
        None known.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  struct Task * ThisTask = FindTask(NULL);

  CloseLibrary((struct Library *)ArpBase);
  
  if (0 == ReturnCode)
  {
    ((struct Process *) ThisTask) -> pr_Result2 = 0;
  }
  else
  {
    ((struct Process *) ThisTask) -> pr_Result2 = fault;
  }  

  Exit(ReturnCode);
  
  AROS_LIBFUNC_EXIT
} /* ArpExit */
