/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Accelerated functions for nVidia cards
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <hidd/graphics.h>

#include "nv.h"
#include "riva_hw.h"

static int ROP[] =
{
	0x00,		// vHidd_GC_DrawMode_Clear
	0x88,		// vHidd_GC_DrawMode_And
	0x44,		// vHidd_GC_DrawMode_AndReverse
	0xcc,		// vHidd_GC_DrawMode_Copy
	0x22,		// vHidd_GC_DrawMode_AndInverted
	0xaa,		// vHidd_GC_DrawMode_NoOp
	0x66,		// vHidd_GC_DrawMode_Xor
	0xee,		// vHidd_GC_DrawMode_Or
	0x11,		// vHidd_GC_DrawMode_Nor
	0x99,		// vHidd_GC_DrawMode_Equiv
	0x55,		// vHidd_GC_DrawMode_Invert
	0xdd,		// vHidd_GC_DrawMode_OrReverse
	0x33,		// vHidd_GC_DrawMode_CopyInverted
	0xbb,		// vHidd_GC_DrawMode_OrInverted
	0x77,		// vHidd_GC_DrawMode_Nand
	0xff		// vHidd_GC_DrawMode_Set
};

inline void wait_for_idle(struct nv_staticdata *nsd)
{
	while (nsd->riva.Busy(&nsd->riva));
}

static void riva_setup_ROP(struct nv_staticdata *nsd, int rop)
{
	RIVA_FIFO_FREE(nsd->riva, Rop, 1);
	nsd->riva.Rop->Rop3 = ROP[rop];
}

void riva_setup_pat(struct nv_staticdata *nsd, int c0, int c1, int p0, int p1)
{
	RIVA_FIFO_FREE(nsd->riva, Patt, 5);
	nsd->riva.Patt->Shape = 0;
	nsd->riva.Patt->Color0 = c0;
	nsd->riva.Patt->Color1 = c1;
	nsd->riva.Patt->Monochrome[0] = p0;
	nsd->riva.Patt->Monochrome[1] = p1;
}

void riva_setup_clip(struct nv_staticdata *nsd, int x0, int y0, int x1, int y1)
{
	RIVA_FIFO_FREE(nsd->riva, Clip, 2);
	nsd->riva.Clip->TopLeft     = (y0 << 16) | x0;
	nsd->riva.Clip->WidthHeight = ((x1 - x0 + 1) << 16) | (y1 - y0 + 1);
}

void riva_setup_accel(struct nv_staticdata *nsd)
{
	riva_setup_ROP(nsd, 3);
	wait_for_idle(nsd);
}

void riva_rectfill(struct nv_staticdata *nsd, int sx,
			  int sy, int width, int height, int color, int rop)
{
	riva_setup_ROP(nsd, rop);

	RIVA_FIFO_FREE(nsd->riva, Bitmap, 3);
	nsd->riva.Bitmap->Color1A = color;
	nsd->riva.Bitmap->UnclippedRectangle[0].TopLeft     = (sx << 16) | sy; 
	nsd->riva.Bitmap->UnclippedRectangle[0].WidthHeight = (width << 16) | height;
}

void riva_line(struct nv_staticdata *nsd, int x0, int y0, int x1, int y1, int color, int rop)
{
	riva_setup_ROP(nsd, rop);
	
	RIVA_FIFO_FREE(nsd->riva, Line, 3);
	nsd->riva.Line->Color = color;
	nsd->riva.Line->Lin[0].point0 = (y0 << 16) || x0;
	nsd->riva.Line->Lin[0].point1 = (y1 << 16) || x1;
}
