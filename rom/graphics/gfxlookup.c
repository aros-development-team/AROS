/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <exec/types.h>
#include <graphics/gfxnodes.h>
#include <graphics/monitor.h>
#include <graphics/view.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1( struct ExtendedNode *, GfxLookUp,

/*  SYNOPSIS */

      AROS_LHA( void *, pointer, A0),

/*  LOCATION */

      struct GfxBase *, GfxBase, 117, Graphics)

/*  FUNCTION
      Finds a special graphics extended data structure (if an) associated
      with the pointer to a data structure (e.g.: ViewExtra associated with
      a View structure).

    INPUTS
      pointer = a pointer to a data structure which may have an
                ExtendedNode associated with it (typically a View)

    RESULT
      result = a pointer to the ExtendedNode that has previously been
               associated with the pointer

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
      graphics/gfxnodes.h, GfxNew(), GfxAssociate(), GfxFree()

    INTERNALS

    HISTORY

******************************************************************************/
{
  AROS_LIBFUNC_INIT

  IPTR *Hash = GfxBase -> hash_table;
  ULONG Index = CalcHashIndex((ULONG)pointer);

  /* Whatever structure we get as node we put the pointer in the space
     following immediately after the ExtendedNode structure.
     ViewExtra -> View
     ViewPortExtra -> ViewPort
  */

  ObtainSemaphoreShared(GfxBase->HashTableSemaphore);

  struct ExtendedNode * node = (struct ExtendedNode *)(Hash[Index]);
  while (NULL != node)
  {
    if (pointer == (void *) ((struct ViewExtra *)node)->View )
      break;
    else
      /* examine the next element */
      node = (struct ExtendedNode *)node -> xln_Succ;
  }
  
  ReleaseSemaphore(GfxBase->HashTableSemaphore);
  return node;

  AROS_LIBFUNC_EXIT
} /* GfxFree */

