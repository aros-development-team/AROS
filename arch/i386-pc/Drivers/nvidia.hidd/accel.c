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

void acc_SetClippingRectangle(struct nv_staticdata *nsd,
		int x1, int y1, int x2, int y2)
{
    int height	= y2-y1 + 1;
    int width 	= x2-x1 + 1;
	
    RIVA_FIFO_FREE(nsd->riva, Clip, 2);
    nsd->riva.Clip->TopLeft	= (y1     << 16) | (x1 & 0xffff);
    nsd->riva.Clip->WidthHeight	= (height << 16) | (width & 0xffff);
}

void acc_DisableClipping(struct nv_staticdata *nsd)
{
    acc_SetClippingRectangle(nsd, 0, 0, 0x7fff, 0x7fff);
}

void acc_SetPattern(struct nv_staticdata *nsd,
		int c1, int c2, int pat1, int pat2)
{
    RIVA_FIFO_FREE(nsd->riva, Patt, 5);
    nsd->riva.Patt->Shape		= 0;	/* 0 -> 8x8 pattern */
    nsd->riva.Patt->Color0		= c1;
    nsd->riva.Patt->Color1		= c2;
    nsd->riva.Patt->Monochrome[0]	= pat1;
    nsd->riva.Patt->Monochrome[1]	= pat2;
}

void acc_SetRop(struct nv_staticdata *nsd, int rop)
{
    acc_SetPattern(nsd, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);	/* Obligatory! */

    RIVA_FIFO_FREE(nsd->riva, Rop, 1);
    nsd->riva.Rop->Rop3 = ROP[rop];
}

void acc_PrepareToFill(struct nv_staticdata *nsd, int color, int rop)
{
    acc_SetRop(nsd, rop);

    RIVA_FIFO_FREE(nsd->riva, Bitmap, 1);
    nsd->riva.Bitmap->Color1A = color;
}

void acc_FillRect(struct nv_staticdata *nsd,
		int x1, int y1, int x2, int y2, int color, int rop)
{
    int height	= y2-y1 + 1;
    int width 	= x2-x1 + 1;

    acc_SetRop(nsd, rop);

    RIVA_FIFO_FREE(nsd->riva, Rect, 3);
    
    nsd->riva.Rect->Color	= color;
    nsd->riva.Rect->TopLeft	= (y1 << 16) | (x1 & 0xffff);
    nsd->riva.Rect->WidthHeight = (height << 16) | (width & 0xffff);
}

void acc_FillRectangle(struct nv_staticdata *nsd,
		int x1, int y1, int x2, int y2)
{
    int height	= y2-y1 + 1;
    int width 	= x2-x1 + 1;

    RIVA_FIFO_FREE(nsd->riva, Bitmap, 2);
    nsd->riva.Bitmap->UnclippedRectangle[0].TopLeft	= (x1 << 16) | (y1 & 0xffff);
    nsd->riva.Bitmap->UnclippedRectangle[0].WidthHeight = (width << 16) | (height & 0xffff);
}

void acc_Sync(struct nv_staticdata *nsd)
{
    RIVA_BUSY(nsd->riva);
}

void acc_SolidLine(struct nv_staticdata *nsd, int color, 
		int x1, int y1, int x2, int y2)
{
    RIVA_FIFO_FREE(nsd->riva, Line, 3);
    nsd->riva.Line->Color = color;
    nsd->riva.Line->Lin[0].point0 = ((y1 << 16) | (x1 & 0xffff));
    nsd->riva.Line->Lin[0].point1 = ((y2 << 16) | (x2 & 0xffff));
}

/* New! */

void new_setupAccel(struct nv_staticdata *nsd)
{
    RIVA_HW_INST *riva = &nsd->riva;
    
    U032 *FIFO   = riva->FIFO;
    U032 *PGRAPH = riva->PGRAPH;
    U032 *PRAMIN = riva->PRAMIN;    

    riva->Rop		= (RivaRop	 *)(FIFO + 0x0000/4);
    riva->Clip		= (RivaClip	 *)(FIFO + 0x2000/4);
    riva->Patt		= (RivaPattern	 *)(FIFO + 0x4000/4);
//    riva.Triangle	= (RivaTriangle	 *)(FIFO + 0x6000/4);
//    riva.Surface	= (RivaSurface	 *)(FIFO + 0x8000/4);
    riva->Rect		= (RivaRectangle *)(FIFO + 0xa000/4);
    riva->Line		= (RivaLine	 *)(FIFO + 0xc000/4);
    riva->Blt		= (RivaScreenBlt *)(FIFO + 0xe000/4);

     PRAMIN[0x00000508] = 0x01008043;   /* Rop         */
     PRAMIN[0x00000509] = 0x00000302;
     PRAMIN[0x0000050A] = 0x00000000;
     PRAMIN[0x0000050B] = 0x00000000;

     PRAMIN[0x0000050C] = 0x01008019;   /* Clip        */
     PRAMIN[0x0000050D] = 0x00000302;
     PRAMIN[0x0000050E] = 0x00000000;
     PRAMIN[0x0000050F] = 0x00000000;

     PRAMIN[0x00000510] = 0x01008018;   /* Pattern     */
     PRAMIN[0x00000511] = 0x00000302;
     PRAMIN[0x00000512] = 0x00000000;
     PRAMIN[0x00000513] = 0x00000000;

     PRAMIN[0x0000051C] = 0x0100A01E;   /* Rectangle   */
     PRAMIN[0x0000051D] = 0x00000302;
     PRAMIN[0x0000051E] = 0x11401140;
     PRAMIN[0x0000051F] = 0x00000000;

     PRAMIN[0x00000520] = 0x0100A01C;   /* Line        */
     PRAMIN[0x00000521] = 0x00000302;
     PRAMIN[0x00000522] = 0x11401140;
     PRAMIN[0x00000523] = 0x00000000;

     PRAMIN[0x00000524] = 0x0100A01F;   /* Blt         */
     PRAMIN[0x00000525] = 0x00000302;
     PRAMIN[0x00000526] = 0x11401140;
     PRAMIN[0x00000527] = 0x00000000;


     /* put objects into subchannels */
     FIFO[0x0000/4] = 0x80000000;  /* Rop         */
     FIFO[0x2000/4] = 0x80000001;  /* Clip        */
     FIFO[0x4000/4] = 0x80000002;  /* Pattern     */
//     FIFO[0x6000/4] = 0x80000010;  /* Triangle    */
//     FIFO[0x8000/4] = 0x80000011;  /* ScaledImage */
     FIFO[0xA000/4] = 0x80000012;  /* Rectangle   */
     FIFO[0xC000/4] = 0x80000013;  /* Line        */
     FIFO[0xE000/4] = 0x80000014;  /* Blt         */


     RIVA_FIFO_FREE( nsd->riva, Patt, 5 );
     nsd->riva.Patt->Shape         = 0; /* 0 = 8X8, 1 = 64X1, 2 = 1X64 */
     nsd->riva.Patt->Color0        = 0xFFFFFFFF;
     nsd->riva.Patt->Color1        = 0xFFFFFFFF;
     nsd->riva.Patt->Monochrome[0] = 0xFFFFFFFF;
     nsd->riva.Patt->Monochrome[1] = 0xFFFFFFFF;

     RIVA_FIFO_FREE( nsd->riva, Rop, 1 );
     nsd->riva.Rop->Rop3 = 0xCC;
}

void new_fillRect(struct nv_staticdata *nsd,
		int x1, int y1, int x2, int y2, int color, int rop)
{
    int height	= y2-y1 + 1;
    int width 	= x2-x1 + 1;

     RIVA_FIFO_FREE( nsd->riva, Rop, 1 );
     nsd->riva.Rop->Rop3 = ROP[rop];

    RIVA_FIFO_FREE(nsd->riva, Rect, 3);
    
    nsd->riva.Rect->Color	= color;
    nsd->riva.Rect->TopLeft	= (y1 << 16) | (x1 & 0xffff);
    nsd->riva.Rect->WidthHeight = (height << 16) | (width & 0xffff);
}
