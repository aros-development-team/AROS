/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(void *, ArpAlloc,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, size_in_bytes, D0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 64, Arp)

/*  NAME
        ArpAlloc -- Allocate memory and track.
 
    SYNOPSIS
        Memory = ArpAlloc( size_in_bytes )
          d0                    d0
 
    FUNCTION
        This function provides a simple memory allocation/tracking
        mechanism. You can allocate memory with this function and,
        when you CloseLibrary(ArpBase) or ArpExit(code), the memory
        will be automatically freed for you.  You can make multiple
        calls to ArpAlloc(), each allocation will be tracked.
 
    INPUTS
        size_in_bytes - Amount of memory required.
 
    RESULT
        Pointer to a memory block with attributes (MEMF_PUBLIC | MEMF_CLEAR),
        or zero, if an error occurred.
 
    WARNINGS/ADDITIONAL CONSIDERATIONS
        REMEMBER: You must call CloseLibrary(ArpBase) for the resource
        freeing to occur.


 
    SEE ALSO
        ArpAllocEntry, FreeTaskResList, ArpOpen, ArpUnLock, ArpDupLock.

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  return ArpAllocMem(size_in_bytes, MEMF_CLEAR|MEMF_PUBLIC);

  AROS_LIBFUNC_EXIT
} /* ArpAlloc */
