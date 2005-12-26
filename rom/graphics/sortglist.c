/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SortGList()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, SortGList,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 25, Graphics)

/*  FUNCTION
	Sort the current gel list by the y and x coordinates of it's
        elements.
        You have to call this routine prior to calling DoCollision()
        of DrawGList or make sure that the list is sorted!

    INPUTS
	rp  = pointer to RastPort that has an GelsInfo linked to it

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() DrawGList() DoCollision() graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

	struct VSprite * CorrectVSprite = rp -> GelsInfo -> gelHead;
	struct VSprite * CurVSprite = CorrectVSprite -> NextVSprite;

	LONG Coord = JOIN_XY_COORDS( CorrectVSprite -> X, CorrectVSprite -> Y );

	while ( NULL != CurVSprite ) {
		LONG NewCoord =  JOIN_XY_COORDS( CurVSprite -> X, CurVSprite -> Y );

		if (0 != CurVSprite->VSBob)
			CurVSprite->VSBob->Flags &= ~BDRAWN;

		if (Coord <= NewCoord) {
			/* sorting is not necessary for this VSprite */
			CorrectVSprite = CurVSprite;
			CurVSprite     = CurVSprite -> NextVSprite;

			Coord = NewCoord;
		} else {
			struct VSprite * tmpVSprite = CorrectVSprite->PrevVSprite;
			/* 
			   The CurVSprite is at the wrong position. It has to appear
			   somewhere earlier in the list.
			 */

			/* unlink it from it's current position */
			CorrectVSprite -> NextVSprite = CurVSprite -> NextVSprite;
			if (NULL != CurVSprite -> NextVSprite)
				CurVSprite -> NextVSprite -> PrevVSprite = CorrectVSprite;

			/* insert CurVSprite at some previous place */

			while ( ( NULL != tmpVSprite ) &&
				 ( JOIN_XY_COORDS(tmpVSprite -> X, tmpVSprite -> Y) > NewCoord ) )
				tmpVSprite = tmpVSprite -> PrevVSprite;

			if (NULL == tmpVSprite) {
				/* our CurVSprite becomes the fist one in the list */
				/* Due to the head VSprite this should never happen!!! */
				rp->GelsInfo->gelHead->PrevVSprite = CurVSprite;
				CurVSprite->NextVSprite = rp->GelsInfo->gelHead;
				CurVSprite->PrevVSprite = NULL;		 
				rp->GelsInfo->gelHead   = CurVSprite;
			} else {
				/* link it into the list *after* the tmpVSprite */
				CurVSprite->PrevVSprite = tmpVSprite;
				CurVSprite->NextVSprite = tmpVSprite->NextVSprite;

				if (NULL != tmpVSprite->NextVSprite)
					tmpVSprite->NextVSprite->PrevVSprite = CurVSprite;

				tmpVSprite->NextVSprite = CurVSprite;
			}

			/* set the new CurVSprite */
			CurVSprite = CorrectVSprite -> NextVSprite;
		}
	} /* while ( NULL != CurVSprite ) */

	rp->GelsInfo->gelTail = CorrectVSprite;

	AROS_LIBFUNC_EXIT
  
} /* SortGList */
