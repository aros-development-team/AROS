/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH0(struct ArpResList *, FindTaskResList,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct ArpBase *, ArpBase, 77, Arp)

/*	NAME 
      FindTaskResList -- find a pointer to current task's reslist

    SYNOPSIS  Finds the Resource List for this task, or NULL
      ResList = FindTaskResList()
	    D0 (A1) ( Zero flag )

    FUNCTION
      This function searches for the most recently nested ResList
      for the current task, if any.  If there are no resource lists
      for this task, the return pointer points at NULL.

    NOTE - This implementation may change.  FindTaskResList is normally
      not needed as an external callout, as all task tracking is
      done automaticly without the application needing to know about
      this

    BUGS
      None known.  Will change to work with SyncRun

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  const struct Task * ThisTask = FindTask(NULL);
  
  struct ArpResList * ARL = (struct ArpResList *)ArpBase->ResLists.mlh_Head;

  while ((ULONG)ThisTask != ARL->TaskID  &&  NULL != ARL)
    ARL = (struct ArpResList *)ARL->ARL_node.mln_Succ;
 
  return ARL;
  
  AROS_LIBFUNC_EXIT
} /* FindTaskResList */
