/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <proto/arp.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH0(BOOL, FreeTaskResList,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct ArpBase *, ArpBase, 62, Arp)

/*  NAME
        FreeTaskResList -- Free tracked resources for this task

    SYNOPSIS
        BOOL = FreeTaskResList()
         d0

    FUNCTION
        This function frees ALL resources tracked by arplibs resource
        tracking mechanism.  This includes memory as well as open
        files and locks.  Ordinarily, you will call this function
        indirectly by CloseLibrary(ArpBase) or ArpExit().  This
        mechanism allows easier exits (whether normal or abnormal)
        than is usually found.


    INPUTS
        NONE

    RESULT
        TRUE if resource deallocation occurred, otherwise FALSE.

    ADDITIONAL
        The tracking scheme has been radically changed from eariler
        versions of arplib, but this should not break any programs.

    BUGS
        None known.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  BOOL freed = FALSE;
  /* Get the pointer to the ArpResList that belongs to this task */  
  struct ArpResList * ARL = FindTaskResList();
 
  /* are there any entries for this task? */
  if (NULL != ARL)
  {
    struct TrackedResource * Last;
    freed = TRUE;
    while (NULL != (Last = (struct TrackedResource* )RemTail((struct List *)&ARL->FirstItem)))
    {
      switch (Last -> TR_ID)
      {
        case TRAK_AAMEM:
          DosFreeMem(Last->TR_Stuff);
        break;
        
        case TRAK_LOCK:
          UnLock((BPTR)Last->TR_Stuff);
        break;
        
        case TRAK_FILE:
          Close((BPTR)Last->TR_Stuff);
        break;

      }
      /* Free this TrackedResource structure */
      FreeMem(Last, sizeof(struct TrackedResource));
    }
    /* Remove te ArpResList structure from the list of ArpResList 
       structures and free it */
    Remove((struct Node *)ARL);
    FreeMem(ARL, sizeof(struct ArpResList));
  }
  
  return freed;
 
  AROS_LIBFUNC_EXIT
} /* FreeTaskResList */
