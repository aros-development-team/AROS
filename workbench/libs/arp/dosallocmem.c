/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/
#include <proto/exec.h>
#include <exec/memory.h>
#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(void *, DosAllocMem,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, size_in_bytes, D0),

/*  LOCATION */
      struct ArpBase *, ArpBase, 57, Arp)

/*  FUNCTION
        This function returns a memory block of the size requested, or
        NULL if the allocation failed.  The memory will satisfy the
        requirements of MEMF_PUBLIC | MEMF_CLEAR.
 
        As expected by AmigaDOS, the total size of the memory block is
        stored at (memblock - 4), so the actual memory allocated will
        always be four bytes larger than size_in_bytes.
 
    INPUTS
        size_in_bytes - the size of the desired block in bytes.
 
    RESULT
        memBlock - a pointer to the allocated free block.  This block
                   will be longword aligned, and the total size of the
                   block is stored at (memblock - 4).  If the allocation
                   failed, memBlock will return zero.
 
    ADDITIONAL CONSIDERATIONS
        The value returned by DosAllocMem is a real pointer.  If you
        need a BPTR, you must convert this value yourself.
 
    BUGS
        None known.
 
    SEE ALSO
        DosFreeMem()
 
    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  ULONG * memBlock;
  
  size_in_bytes += 4;

  memBlock = AllocMem(size_in_bytes, MEMF_PUBLIC|MEMF_CLEAR);
  memBlock[0] = size_in_bytes;
  
  return (void *)&memBlock[1]; 
  AROS_LIBFUNC_EXIT
} /* DosAllocMem */
