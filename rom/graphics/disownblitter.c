/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release the bitter from private usage
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <graphics/gfxbase.h>
#include <exec/tasks.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH0(void, DisownBlitter,
        
/*  SYNOPSIS */


/*  LOCATION */
        struct GfxBase *, GfxBase, 77, Graphics)

/*  FUNCTION
        The blitter is returned to usage by other tasks.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        DisownBlitter()

    INTERNALS

    HISTORY

******************************************************************************/
{
  AROS_LIBFUNC_INIT

  /* Debugging disabled, this function must be callable from blitter interrupt.
   * FIXME: better solution?
   */

#if 0
  struct Task *me = FindTask(NULL);

  D(bug("DisownBlitter: Release by Task %p\n", me));

  if (GfxBase->BlitOwner != me) {
    D(bug("DisownBlitter: Owned by Task %p, but Task %p is releasing it!\n",
      GfxBase->BlitOwner, me));
  }

  if (NULL != GfxBase->blthd && NULL != GfxBase->bsblthd)
  {
    D(bug("DisownBlitter: OOPS! Disowning while queued enties are in play!\n"));
  }
#endif

  Disable();

  GfxBase->BlitOwner = NULL;
  /* Do we have any waiting tasks? */
  if (!IsListEmpty(&GfxBase->BlitWaitQ)) {
    /* Wake up next OwnBlitter() waiting task */
    struct BlitWaitQNode *node = (struct BlitWaitQNode*)GfxBase->BlitWaitQ.lh_Head;
    D(bug("DisownBlitter: Waking task %p\n", node->task));
    Signal(node->task, 1 << SIGB_BLIT);
  }

  Enable();

#if 0
  D(bug("DisownBlitter: Released by Task %p\n", me));
#endif

  AROS_LIBFUNC_EXIT
} /* DisownBlitter */
