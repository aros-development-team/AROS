/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Common routines for Gadgets
    Lang: english
*/
#include "gadgets.h"
#include <intuition/intuition.h>

/* Calculate the size of the Bounding Box of the gadget */
void CalcBBox (struct Window * window, struct Gadget * gadget,
	struct BBox * bbox)
{
#define ADDREL(flag,field)  ((gadget->Flags & (flag)) ? window->field : 0)

    bbox->Left	 = ADDREL(GFLG_RELRIGHT,Width)   + gadget->LeftEdge;
    bbox->Top	 = ADDREL(GFLG_RELBOTTOM,Height) + gadget->TopEdge;
    bbox->Width  = ADDREL(GFLG_RELWIDTH,Width)   + gadget->Width;
    bbox->Height = ADDREL(GFLG_RELHEIGHT,Height) + gadget->Height;
} /* CalcBBox */

