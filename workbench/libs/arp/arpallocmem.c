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

      AROS_LH2(void *, ArpAllocMem,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, size, D0),
      AROS_LHA(ULONG, reqs, D1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 65, Arp)

/*    NAME
        ArpAllocMem -- Allocate and track memory.
 
    SYNOPSIS
        MemPtr = ArpAlloc( size, reqs )
          d0 (a1)	    D0 / D1
 
    FUNCTION
 
        This function is identical to the Exec AllocMem call, but will
        track memory resources for you.
 
        When you CloseLibrary(ArpBase), any memory allocated with this
        function will be freed, which provides a simpler means of
        termination than is ordinarily found.
 
        You may make multiple calls to this routine -- all memory
        resources will be tracked.  Note also that this function
        requires you to specify the type of memory, so that you may
        also allocate and track CHIP memory using this function.
 
    INPUTS
        Same args as Exec AllocMem request, size / reqs
 
    RESULT
        MemPtr -- pointer to the memory requested, same as Exec AllocMem
   		  return.  If this is NON-ZERO, the memory allocation
        succeeded.
        Tracker -- same as all the tracking calls, register A1 contains
        the pointer to the TRACKER.  See ArpAlloc, GetTracker,
        CreateTaskReslist,FreeTaskReslist.
 
    WARNING
        Do NOT call FreeMem to free the memory from this allocation!
        If you want to free this allocation before terminating the task,  
        or before calling FreeTaskReslist, you MUST use the TRACKER pointer
        and use FreeTrackedItem.
 
        If you want to manage your own memory, use the exec calls directly.
 
    BUGS
        None known.
 
    SEE ALSO
        ArpAlloc(),  GetTracker(), FreeTaskResList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  ULONG * MemPtr;
  size += 4;
  MemPtr = (ULONG *)AllocMem(size, reqs);
  MemPtr[0] = size;
  
  /* This memory could later be freed via DosFreeMem() */
  intern_AddTrackedResource(ArpBase, TRAK_AAMEM, (APTR)&MemPtr[1]);  

  return &MemPtr[1];
  AROS_LIBFUNC_EXIT
} /* ArpAllocMem */
