/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: gadget box with relative geometry resolved (GFLG_RELRIGHT,
          GFLG_RELBOTTOM, GFLG_RELWIDTH, GFLG_RELHEIGHT), the way intuition
          resolves it against the gadget's domain
*/

#ifndef REACTION_BOX_H
#define REACTION_BOX_H

#include <intuition/intuition.h>
#include <intuition/cghooks.h>

static inline void reaction_gadget_box(struct Gadget *g, struct GadgetInfo *gi,
                                       WORD *x, WORD *y, WORD *w, WORD *h)
{
    WORD dw = gi ? gi->gi_Domain.Width : 0, dh = gi ? gi->gi_Domain.Height : 0;

    *x = g->LeftEdge + ((g->Flags & GFLG_RELRIGHT)  ? dw - 1 : 0);
    *y = g->TopEdge  + ((g->Flags & GFLG_RELBOTTOM) ? dh - 1 : 0);
    *w = g->Width    + ((g->Flags & GFLG_RELWIDTH)  ? dw : 0);
    *h = g->Height   + ((g->Flags & GFLG_RELHEIGHT) ? dh : 0);
}

#endif /* REACTION_BOX_H */
