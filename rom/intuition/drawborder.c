/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/23 17:28:18  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <clib/graphics_protos.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	__AROS_LH4(void, DrawBorder,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A0),
	__AROS_LHA(struct Border   *, border, A1),
	__AROS_LHA(long             , leftOffset, D0),
	__AROS_LHA(long             , topOffset, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 18, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG  apen;
    ULONG  bpen;
    ULONG  drmd;
    WORD * ptr;
    WORD   x, y;
    int    t;

    apen = GetAPen (rp);
    bpen = GetBPen (rp);
    drmd = GetDrMd (rp);

    for ( ; border; border=border->NextBorder)
    {
	SetAPen (rp, border->FrontPen);
	SetBPen (rp, border->BackPen);
	SetDrMd (rp, border->DrawMode);

	Move (rp
	    , x = border->LeftEdge + leftOffset
	    , y = border->TopEdge + topOffset
	);

	ptr = border->XY;

	for (t=0; t<border->Count; t++)
	{
	    x += *ptr ++;
	    y += *ptr ++;

	    Draw (rp, x, y);
	}
    }

    SetAPen (rp, apen);
    SetBPen (rp, bpen);
    SetDrMd (rp, drmd);

    __AROS_FUNC_EXIT
} /* DrawBorder */
