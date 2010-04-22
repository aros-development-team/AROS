/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxnodes.h>
#include <graphics/monitor.h>
#include <graphics/view.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */

      AROS_LH1( void , GfxFree,

/*  SYNOPSIS */
      AROS_LHA(struct ExtendedNode *, node, A0),

/*  LOCATION */

      struct GfxBase *, GfxBase, 111, Graphics)

/*  FUNCTION
      Free a special graphics extended data structure which was preciously
      allocated by GfxNew()

    INPUTS
      node = pointer to a graphics extended data structure obtained by
             GfxNew()

    RESULT
      The node will be deallocated from memory. Graphics will disassociate
      this special graphics extended node from any associated data
      structure, if necessary, before freeing it (see GfxAssociate())

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
      graphics/gfxnodes.h, GfxNew(), GfxAssociate(), GfxLookUp()

    INTERNALS

    HISTORY

******************************************************************************/
{
  AROS_LIBFUNC_INIT

  const ULONG GfxNew_memsizes[] = { 0,
                                    sizeof(struct ViewExtra),
                                    sizeof(struct ViewPortExtra),
                                    sizeof(struct SpecialMonitor),
                                    sizeof(struct MonitorSpec)
                                  };

  D(bug("GfxFree(0x%p)\n", node));
  if ( SS_GRAPHICS == node->xln_Subsystem &&
       NT_GRAPHICS == node->xln_Type)
  {
    /* take the element out of the hashlist, if it is in the
       hashlist  */
    D(bug("[GfxFree] xln_Succ 0x%p, xln_Pred 0x%p\n", node->xln_Succ, node->xln_Pred));

    ObtainSemaphore(GfxBase->HashTableSemaphore);

    /* if the element has a Successor */
    if (NULL != node -> xln_Succ)
      ((struct ExtendedNode *)(node -> xln_Succ)) -> xln_Pred = (struct Node *) (node -> xln_Pred);

    /* if the previous Element is not the hashlist itself */
    /* (the same code works also if the previous entry is the hashlist ) */
    if (node->xln_Pred)
        ((struct ExtendedNode *)(node -> xln_Pred)) -> xln_Succ = (struct Node *) (node -> xln_Succ);

    ReleaseSemaphore(GfxBase->HashTableSemaphore);

    FreeMem((void *) node, GfxNew_memsizes[node->xln_Subtype]);
  }

  AROS_LIBFUNC_EXIT
} /* GfxFree */

