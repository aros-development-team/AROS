/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/monitor.h>
#include <graphics/view.h>
#include <graphics/gfxnodes.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH2( void , GfxAssociate,

/*  SYNOPSIS */

      AROS_LHA( void *, pointer, A0),
      AROS_LHA( struct ExtendedNode *, node, A1),

/*  LOCATION */

      struct GfxBase *, GfxBase, 112, Graphics)

/*  FUNCTION
      Associate a special graphics extended data structure with another
      structure via the other structure's pointer. Later, when you call
      GfxLookUp() with the other structure's pointer you may recieve
      the pointer to this special graphics extended data strcuture, if it
      is available

    INPUTS
      pointer = a pointer to a data structure
      node = an ExtendedNode strcuture to associate with the pointer

    RESULT
      an association is created between the pointer and the node such
      that given the pointer the node can be retrieved via GfxLookUp().

    NOTES
      Never associate one special graphics extended data structure to
      several pointers. Only one pointer is allowed!

    EXAMPLE

    BUGS

    SEE ALSO
      graphics/gfxnodes.h GfxFree(), GfxNew(), GfxLookUp()

    INTERNALS

    HISTORY

******************************************************************************/
{
  AROS_LIBFUNC_INIT

  LONG * Hash = GfxBase -> hash_table;
  ULONG Index = CalcHashIndex((ULONG)pointer);

  /* Whatever structure we get as node we put the pointer in the space
     following immediately after the ExtendedNode structure.
     ViewExtra -> View          is equal to
     ViewPortExtra -> ViewPort
  */

  ((struct ViewExtra *)node) -> View = pointer;
        Forbid();
          /* Insert the structure into a hash_list which is to be found
             in the gfxlibrary */
          if (0L != Hash[Index])
            ((struct ExtendedNode *)Hash[Index]) -> xln_Pred = (struct Node *)node;
          node -> xln_Succ = (struct Node *)Hash[Index];
          node -> xln_Pred = (struct Node *)&Hash[Index];
          Hash[Index] = (LONG)node;
        Permit();

  AROS_LIBFUNC_EXIT
} /* GfxAssociate */
