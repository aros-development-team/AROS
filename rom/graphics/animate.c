/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function Animate()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(void, Animate,

/*  SYNOPSIS */
	AROS_LHA(struct AnimOb **, anKey, A0),
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 27, Graphics)

/*  FUNCTION
	Animate every AnimOb in the list. In particular do the following:
	- update location and velocities
	- call AnimOb's special routine if supplied
	- for every component of the Anim ob do:
	  - switch to new sequence if current sequence times out
	  - call the special routine of the component if supplied
	  - set the the coordinates of the VSprite of this
	    sequence to whatever these routines cause

    INPUT
	anKey = address of a pointer to the firts AnimOb in the list
                (same address that was passed to AddAnimOb!)
	rp    = pointer to a valid RastPort structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddAnimOb() graphics/rastport.h graphics/gels.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct AnimOb * CurAnimOb = *anKey;
  struct AnimComp * CurAnimComp;

  /* Animate every object in the list */
  while (NULL != CurAnimOb)
  {
    /* advance the clock */
    CurAnimOb -> Clock += 1;

    /* store old position */
    CurAnimOb -> AnOldY = CurAnimOb -> AnY;
    CurAnimOb -> AnOldX = CurAnimOb -> AnX;

    CurAnimOb -> AnY += CurAnimOb -> YVel;
    CurAnimOb -> AnX += CurAnimOb -> XVel;

    CurAnimOb -> YVel += CurAnimOb -> YAccel;
    CurAnimOb -> XVel += CurAnimOb -> XAccel;

    /* Call the special routine of this AnimOb */
    if (NULL != CurAnimOb -> AnimORoutine)
      (CurAnimOb -> AnimORoutine)();

    /* Now let's animate all it's components */
    CurAnimComp = CurAnimOb -> HeadComp;

    while (NULL != CurAnimComp)
    {
      long coord;
      struct AnimComp * CurSeqAnimComp = CurAnimComp;
      struct VSprite * CurVSprite, * NewVSprite;

      /* decrease the timer */
      CurSeqAnimComp -> Timer -= 1;

      /* if the timer is 0 then the sequence has timed out ->
         prepare the next sequence and set the bob of the current
         sequence to be removed from the gel list. */

      CurVSprite = CurSeqAnimComp -> AnimBob -> BobVSprite;

      if (0 == CurSeqAnimComp -> Timer)
      {
        /* if this was the first component of the AnimOb then
           change the head-pointer to the next sequence */
        if (CurSeqAnimComp == CurAnimOb -> HeadComp)
          CurAnimOb -> HeadComp = CurSeqAnimComp -> NextSeq;

        /* initilize the NextComp and PrevComp pointers of the next
           sequence to the values of the current sequence */
        CurSeqAnimComp -> NextSeq -> NextComp = CurSeqAnimComp -> NextComp;
        CurSeqAnimComp -> NextSeq -> PrevComp = CurSeqAnimComp -> PrevComp;

        /* initilize the NextComp pointer of the previous component
           - if existing - to point to the next sequence */
        if (CurSeqAnimComp -> PrevComp)
          CurSeqAnimComp -> PrevComp -> NextComp = CurSeqAnimComp -> NextSeq;

        /* initilize the PrevComp pointer of the next component
           - if existing - to point to the next sequence */
        if (CurSeqAnimComp -> NextComp)
          CurSeqAnimComp -> NextComp -> PrevComp = CurSeqAnimComp -> NextSeq;

        /* init the timer of the next sequence */

        CurSeqAnimComp -> NextSeq -> Timer = CurSeqAnimComp -> NextSeq -> TimeSet;
        AddBob(CurSeqAnimComp -> NextSeq -> AnimBob, rp);

        /* get the VSprite of the new sequence, we need it a few times */
        NewVSprite = CurSeqAnimComp -> NextSeq -> AnimBob -> BobVSprite;

        NewVSprite -> OldY = CurVSprite -> Y;
        NewVSprite -> OldX = CurVSprite -> X;

        /* as this sequence is complete we might have to add the
         * RingX/YTrans to AnX/Y if the appropriate flags was set
	 */
        if (0 != (CurSeqAnimComp -> Flags & RINGTRIGGER))
        {
          CurAnimOb -> AnY += CurAnimOb -> RingYTrans;
          CurAnimOb -> AnX += CurAnimOb -> RingXTrans;
        }

	/* calculate the coordinates of the VSprite
         * here [0.5 .. 1.4999] is rounded to 1
	 *	[1.5 .. 2.4999] is rounded to 2 and so on
         */
        coord = (CurSeqAnimComp -> NextSeq -> YTrans + CurAnimOb -> AnY ) >> 5;
        /* for better accuracy */
        if (0 != (coord & 1))
         	coord += 2;
        NewVSprite -> Y = (coord >> 1);

        coord = (CurSeqAnimComp -> NextSeq -> XTrans + CurAnimOb -> AnX ) >> 5;
        /* for better accuracy */
        if (0 != (coord & 1))
         	coord += 2;
        NewVSprite -> X = (coord >> 1);

        CurVSprite -> Y = 0x8001;
        CurVSprite -> X = 0x8001;
        /* Remove the Bob from the gel list =
           Mark the bob of the current sequence to be removed during
           the next call of DrawGList(). DrawGList() will first
           restore the old background where the bob is now and then
           unlink the bob from the gel list. If double buffering is
           used it will take two calls to DrawGList() to remove the
           bob completely from the list. */
        RemBob(CurSeqAnimComp -> AnimBob);
      }
      else
      {
       /* move (animate) the bob (actually the VSprite) that belongs
         to this sequence */
       /* do NOT change OldY/X -> leave this up to DrawGList() */
        coord = ( CurSeqAnimComp -> YTrans +
                  CurAnimOb      -> AnY ) >> 5;
        /* for better accuracy */
        if (0 != (coord & 1))
         	coord += 2;
        CurVSprite -> Y = (coord >> 1);

        coord = ( CurSeqAnimComp -> XTrans +
                  CurAnimOb -> AnX      ) >> 5;
        /* for better accuracy */
        if (0 != (coord & 1))
         	coord += 2;
        CurVSprite -> X = (coord >> 1);

      }

      /* call this componentent's special routine if supplied
       * !!! this routine might give me a result but I don't know
       * for sure and what would I do with the result???
       */
      if (NULL != CurSeqAnimComp -> AnimCRoutine)
        (CurSeqAnimComp -> AnimCRoutine)();

      /* animate the next component of this object in the list */
      CurAnimComp = CurAnimComp -> NextComp;
    }

    /* advance to the next object in list */
    CurAnimOb = CurAnimOb -> NextOb;
  }

  AROS_LIBFUNC_EXIT
} /* Animate */
