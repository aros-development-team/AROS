/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH0(struct ArpResList *, CreateTaskResList,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct ArpBase *, ArpBase, 78, Arp)

/*    NAME
      CreateTaskResList -- Create a new nested ResList for this task

    SYNOPSIS
      ResList = CreateTaskResList()
      D0 / Z Flag

    FUNCTION
      Create a new Resource list for this task, and insert it at the
      HEAD of the ResourceArray list.

      You do not normally need to use CreateTaskResList, because the
      functions which insert tracked items into the task reslist
      will automaticly create a reslist if one did not exist before.

      This function may be used to explictly create a nested ResList
      for this task; all resources allocated after CreateTaskReslist
      will be stored in the new ResList until you call FreeTaskResList.
      This would allow you, for instance, to call CreateTaskResList at
      the start of a function, and FreeTaskResList at the end of the
      function, and any resources tracked in the interum would be
      freed; but all other tracked resources for the task would
      remain tracked.

      All Task reslists will also be automaticly freed when you call

    BUGS
	    None known.  Current implementation will change slightly when
	    SyncRun is added to arp.library
    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  /* Get memory for the ResList */
  struct ArpResList * ARL = AllocMem(sizeof(struct ArpResList),MEMF_CLEAR|MEMF_PUBLIC );
  ARL -> TaskID = (ULONG)FindTask(NULL);

  /* Init the List within the ArpResList */
  ARL -> FirstItem.mlh_TailPred = (struct MinNode *)&ARL -> FirstItem.mlh_Head;
  ARL -> FirstItem.mlh_Head     = (struct MinNode *)&ARL -> FirstItem.mlh_Tail;

  /* Link it into the ResourceArray list*/
  if (NULL != ARL)
    AddHead((struct List*)&ArpBase->ResLists, (struct Node *)ARL);
  
  return ARL;

  AROS_LIBFUNC_EXIT
} /* CreateTaskResList */
