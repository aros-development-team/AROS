/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Basic help functions needed by gadtools.library.
    Lang: English.
*/
#include <exec/types.h>
#include <intuition/screens.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <aros/debug.h>
#include "gadtools_intern.h"

void drawbevelsbyhand(struct GadToolsBase_intern *GadToolsBase,
                      struct RastPort *rport,
                      WORD left, WORD top, WORD width, WORD height,
                      struct TagItem *taglist)
{
    struct VisualInfo *vi;
    UWORD pen1, pen2;

    vi = (struct VisualInfo *)GetTagData(GT_VisualInfo, NULL, taglist);
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
}
