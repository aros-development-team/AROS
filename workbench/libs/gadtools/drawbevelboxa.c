/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <intuition/screens.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <libraries/gadtools.h>
#include <graphics/rastport.h>
#include <utility/tagitem.h>

	AROS_LH6(void, DrawBevelBoxA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rport, A0),
	AROS_LHA(WORD, left, D0),
	AROS_LHA(WORD, top, D1),
	AROS_LHA(WORD, width, D2),
	AROS_LHA(WORD, height, D3),
	AROS_LHA(struct TagItem *, taglist, A1),

/*  LOCATION */
	struct Library *, GadToolsBase, 20, GadTools)

/*  FUNCTION
	DrawBevelBoxA() does just that. It draws a bevelled box.

    INPUTS
	rport   - rastport, in which the box should be drawn
	left    - left edge of the box
	top     - top edge of the box
	width   - width of the box
	height  - height og the box
	taglist - additional tags

    RESULT

    NOTES
	Boxes drawn with DrawBevelBox() aren't refreshed automatically.
	You have to refresh them yourself.
	The rastport will be modified for DrawBevelBoxA() to guarantee
	fast drawing.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)
    struct VisualInfo *vi;
    UWORD pen1, pen2;

    vi = (struct VisualInfo *)GetTagData(GT_VisualInfo, NULL, taglist);
    if (vi == NULL)
	return;
    if (((BOOL)GetTagData(GTBB_Recessed, FALSE, taglist)) == FALSE)
    {
	pen1 = vi->vi_dri->dri_Pens[SHINEPEN];
	pen2 = vi->vi_dri->dri_Pens[SHADOWPEN];
    } else
    {
	pen1 = vi->vi_dri->dri_Pens[SHADOWPEN];
	pen2 = vi->vi_dri->dri_Pens[SHINEPEN];
    }

    SetDrMd(rport, JAM1);
    switch (GetTagData(GTBB_FrameType, BBFT_BUTTON, taglist))
    {
    case BBFT_BUTTON:
	SetAPen(rport, pen2);
	Move(rport, left + width, top);
	Draw(rport, left + width, top + height);
	Draw(rport, left, top + height);
	SetAPen(rport, pen1);
	Draw(rport, left, top);
	Draw(rport, left + width, top);
	break;
    case BBFT_RIDGE:
	SetAPen(rport, pen2);
	Move(rport, left + 1, top + height);
	Draw(rport, left + 1, top + 1);
	Draw(rport, left + width, top + 1);
	Move(rport, left + 2, top + height);
	Draw(rport, left + width, top + height);
	Draw(rport, left + width, top + 2);
	SetAPen(rport, pen1);
	Move(rport, left, top + height);
	Draw(rport, left, top);
	Draw(rport, left + width, top);
	Move(rport, left + 2, top + height - 1);
	Draw(rport, left + width - 1, top + height - 1);
	Draw(rport, left + width - 1, top + 2);
	break;
    case BBFT_ICONDROPBOX:
	SetAPen(rport, pen2);
	Move(rport, left + width, top);
	Draw(rport, left + width, top + height);
	Draw(rport, left, top + height);
	SetAPen(rport, pen1);
	Draw(rport, left, top);
	Draw(rport, left + width, top);
	Move(rport, left + width - 2, top + 2);
	Draw(rport, left + width - 2, top + height - 2);
	Draw(rport, left + 2, top + height - 2);
	SetAPen(rport, pen2);
	Draw(rport, left + 2, top + 2);
	Draw(rport, left + width - 2, top + 2);
	break;
    }

    AROS_LIBFUNC_EXIT
} /* DrawBevelBoxA */
