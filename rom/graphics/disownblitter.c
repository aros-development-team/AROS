/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release the bitter from private usage
    Lang: english
*/

#define DEBUG 0
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
  /* if there are no blits in any of the two queues (QBlit() and QBSBlit())
     then check whether there is a Task in the BlitWaitQ and put it into
     the TaskReadyList.
   */

  AROS_LIBFUNC_INIT

  if (GfxBase->BlitOwner != FindTask(NULL)) {
      D(bug("DisownBlitter: Owned by Task %p, but Task %p is releasing it!\n",
      	      GfxBase->BlitOwner, FindTask(NULL)));
  }

  if (NULL != GfxBase->  blthd &&
      NULL != GfxBase->bsblthd)
  {
    D(bug("DisownBlitter: OOPS! Disowning while queued enties are in play!\n"));
  }

  Disable();
  GfxBase->BlitOwner = NULL;
  Enable();

  ULOCK_BLIT;

  D(bug("DisownBlitter: Released by Task %p\n", FindTask(NULL)));

  AROS_LIBFUNC_EXIT
} /* DisownBlitter */
