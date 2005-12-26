/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Queue a Blit
    Lang: english
*/

#include <proto/exec.h>
#include <hardware/blit.h>
#include <graphics/gfxbase.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, QBlit,
        
/*  SYNOPSIS */
        AROS_LHA(struct bltnode *, bn, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 46, Graphics)

/*  FUNCTION
        Queus a request for a blit. This request is queued at the end
        of the list.

    INPUTS
        bn - pointer to blitnode structure

    RESULT
        The routine that function in the bltnode is pointing to is
        called when the blitter is ready for work. No other task will
        be able to access the blitter while you're doing the blit.
        Queued blits have precedence over a task that tries to own the
        blitter via OwnBlitter(). So all queued blitter requests will
        be done first until the task that attempts a OwnBlitter can
        actually access the blitter.

    NOTES
        Not all hardware has a blitter. On hardware where there is no
        blitter, a blitter is simulated. Therefore all code that will
        be executed in the function that is called must not contain
        code that is hacking the blitter's register but should contain
        calls to graphics functions instead.      

    EXAMPLE

    BUGS

    SEE ALSO
        QBSBlit() OwnBlitter DisownBlitter() hardware/blit.h

    INTERNALS

    HISTORY

******************************************************************************/
{
  AROS_LIBFUNC_INIT

  /* this function uses the simple FIFO queue blthd (blttl) */
  
  /* I am accessing a public structure and there's no semaphore...*/
  Forbid();
  
  if (NULL == GfxBase->blthd)
  { 
    /* it's the first one in the list */
    GfxBase->blthd = bn;
    GfxBase->blttl = bn;
      
    /* In this case the following also has to happen: 
       It is my understanding that at the end of every blit an interrupt
       occurs that can take care of any blits in this queue or allow
       a taks to wake up when it was blocked due to a call to OwnBlitter.
       But in this case there might not be such an interrupt for a long
       time if no calls to blitterfunctions are made. Therefore this
       blit might be queued forever. To avoid this I have to cause
       a Blitter interrupt, if no task owns the blitter right now.
       (BlitOwner) 
    */
    /*
      !!! missing code here!! See explanation above!
    */
  }
  else
  {
    /* queue it at the end */
    GfxBase->blttl->n = bn;
    GfxBase->blttl    = bn;
  }

  Permit();

  AROS_LIBFUNC_EXIT
} /* QBlit */
