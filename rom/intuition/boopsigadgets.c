/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Routines for BOOL Gadgets
    Lang: english
*/

#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>

void RefreshBoopsiGadget (struct Gadget * gadget, struct Window * win,
	struct IntuitionBase * IntuitionBase)
{
    struct gpRender gpr;

    gpr.MethodID   = GM_RENDER;
    gpr.gpr_Redraw = GREDRAW_REDRAW;

    DoGadgetMethodA (gadget, win, NULL, (Msg)&gpr);
} /* RefreshBoopsiGadget */
