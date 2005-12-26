/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release the bitter from private usage
    Lang: english
*/

#include <proto/exec.h>
#include <graphics/gfxbase.h>
#include <exec/tasks.h>

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

  Forbid();

  GfxBase -> BlitOwner = NULL;
  
  if (NULL == GfxBase->  blthd &&
      NULL == GfxBase->bsblthd)
  {
     if((struct Node *) GfxBase->BlitWaitQ.lh_Head != 
        (struct Node *)&GfxBase->BlitWaitQ.lh_TailPred )
     {
       /* make that task ready again! */
       struct Task * first = (struct Task *)RemHead(&GfxBase->BlitWaitQ);
       first->tc_State = TS_READY;
       /* Put it into the correct list of tasks */
       Reschedule(first);
     }
  } 
  else
  {
    /* let the interrupt handler start the queued blitter requests */
  }
  
  Permit();

  AROS_LIBFUNC_EXIT
} /* DisownBlitter */
