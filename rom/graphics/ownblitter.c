/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Try to own the blitter for private usage
    Lang: english
*/

#include <proto/exec.h>
#include <graphics/gfxbase.h>
#include <exec/execbase.h>
#include <exec/tasks.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH0(void, OwnBlitter,
        
/*  SYNOPSIS */


/*  LOCATION */
        struct GfxBase *, GfxBase, 76, Graphics)

/*  FUNCTION
        The blitter is allocated for excludive use by the calling task.
        This function returns immediatedly if no other task is using
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

  /* prevent other tasks from doing what I am doing */
  struct Task * me;
  
  me  = FindTask(NULL);

  Forbid();
  
  /* test whether a task is using the blitter. Even if the blitter is
     used by the queued blits now the BlitOwner entry must not be NULL!
   */
  
  if (NULL == GfxBase->BlitOwner)
  {
    /* nobody is using the blitter right now, so I can use it */
    GfxBase->BlitOwner=me;
  }
  else
  {
    BOOL first = TRUE;
    /* the blitter is used. I have to set this task asleep and queue
       it into the BlitWaitQ. 
    */
    
    /* Repeat this as long as there is somebody else using the blitter.
       This is necessary as when the other task calls DisownBlitter() it
       might take a while until this task gets to run again and 
       yet another taks might issue QBlit() in the meantime and the blitter 
       might be busy with that. So this task will have to wait again.
       However at the first call this task is put to the end of the waiting
       list and after that it is always put at the very front.
    */
    while (NULL != GfxBase->BlitOwner)
    {
      /* force this task to sleep */
    
      BYTE old_TDNestCnt = SysBase->TDNestCnt;
      SysBase->TDNestCnt=-1;

      /* 
         Move it to the waiting list in the GfxBase structure.
         It will be moved to the ready list by the blitterinterrupt 
         handler. 
      */
      if (TRUE == first)
      {
        AddTail(&GfxBase->BlitWaitQ, &me->tc_Node);
        /* The next time I will put this task  at the beginning
           of the list, if necessary. 
        */
        first = FALSE;
      }
      else
      {
        AddHead(&GfxBase->BlitWaitQ, &me->tc_Node);
      }
      
      /* Switch to the next ready task. */
      Switch();
      /*
        OK. Somebody awakened me. This means that I the task might now
        have full control over the blitter. Checking is in the while-loop.
      */
    
      /* Restore TDNestCnt. */
      SysBase->TDNestCnt=old_TDNestCnt;
    } /* while () */
    /* I am the owner now !! */
    Disable();
    GfxBase -> BlitOwner = me;
    Enable();
  }
  
  Permit();

  AROS_LIBFUNC_EXIT
} /* OwnBlitter */
