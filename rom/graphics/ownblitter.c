/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Try to own the blitter for private usage
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <graphics/gfxbase.h>
#include <exec/execbase.h>
#include <exec/tasks.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH0(void, OwnBlitter,
        
/*  SYNOPSIS */


/*  LOCATION */
        struct GfxBase *, GfxBase, 76, Graphics)

/*  FUNCTION
        The blitter is allocated for exclusive use by the calling task.
        This function returns immediately if no other task is using
        the blitter right now or if no blits are in the queues (QBlit(),
        QBSBlit()). Otherwise the function will block until the blitter
        can be accessed.
        It is good practice to start the blitter immediately after calling 
        this function and then call DisownBlitter() so other tasks can
        use the blitter. 

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

  struct BlitWaitQNode node;
  struct Task *me;
  
  me = FindTask(NULL);

  D(bug("OwnBlitter: Request by Task %p)\n", me));

  Disable();
  for (;;) {
    if (GfxBase->BlitOwner == NULL) {
      GfxBase->BlitOwner = me;
      Enable();
      D(bug("OwnBlitter: Now owned by Task %p\n", me));
      return;
    }
    node.task = me;
    AddTail(&GfxBase->BlitWaitQ, (struct Node*)&node);
    SetSignal(0, 1 << SIGB_BLIT);
    Wait(1 << SIGB_BLIT);
    Remove((struct Node*)&node);
  }

  AROS_LIBFUNC_EXIT
} /* OwnBlitter */
