/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <intuition/screens.h>
#include <proto/graphics.h>

#include "main.h"
#include "support.h"

void RenderFrame(struct RastPort *rp, struct DrawInfo *dri, WORD x1, WORD y1, WORD x2, WORD y2, BOOL recessed)
{
    WORD pen1 = dri->dri_Pens[SHINEPEN];
    WORD pen2 = dri->dri_Pens[SHADOWPEN];
    
    if (recessed)
    {
        pen1 ^= pen2;
	pen2 ^= pen1;
	pen1 ^= pen2;
    }
    
    SetDrMd(rp, JAM2);
    SetAPen(rp, pen1);
    
    RectFill(rp, x1, y1, x2, y1);
    RectFill(rp, x1, y1 + 1, x1, y2);
    
    SetAPen(rp, pen2);
    
    RectFill(rp, x2, y1 + 1, x2, y2);
    RectFill(rp, x1 + 1, y2, x2 - 1, y2);
}

