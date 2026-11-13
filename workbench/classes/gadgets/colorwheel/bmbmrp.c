/*
 * $Id$
 *
 * :ts=4
 *
 * Smart replacement for graphics.library/BltMaskBitMapRastPort
 * Copyright © 1997 by Olaf Barthel, All Rights Reserved
 *
 * Compile with (SAS/C 6.58):
 *
 *    sc link nostartup data=faronly strmerge nostkchk
 *       nodebug opttime optimize bmbmrp.c
 */

#include <graphics/rastport.h>
#include <hardware/blit.h>
#include <utility/hooks.h>

#include <proto/graphics.h>
#include <proto/layers.h>

#include "CompilerSpecific.h"

/****************************************************************************/

#define MINTERM_B_EQUALS_C (ABC|ANBNC|NABC|NANBNC)
#define NUM_ELEMENTS(t) (sizeof(t) / sizeof(t[0]))

/******************************************************************************/

struct LayerMsg
{
	struct Layer *		Layer;
	struct Rectangle	Bounds;
	LONG				OffsetX;
	LONG				OffsetY;
};

struct BltArgs
{
	struct Hook			Hook;
	struct BitMap		MaskBitMap;
	struct BitMap *		SrcBitMap;
	WORD				SrcX,SrcY;
	WORD				DstX,DstY;
	WORD				SizeX,SizeY;
	UBYTE				MinTerm;
	struct Library *	GfxBase;
};

/* This routine is called for every single clipping
 * region in the destination RastPort.
 */
STATIC VOID
FillRoutine(
	REG(a0, struct Hook *		Hook ),
	REG(a1, struct LayerMsg *	Bounds ),
	REG(a2, struct RastPort *	RPort ) )
{
	struct Library * GfxBase;
	struct BltArgs * Args;
	WORD SrcX,SrcY;
	WORD DstX,DstY;
	WORD SizeX,SizeY;

	Args = Hook->h_Data;
	GfxBase = Args->GfxBase;

	SrcX = Args->SrcX;
	SrcY = Args->SrcY;

	/* If this is a layered RastPort, adjust for the
	 * region offset.
	 */
	if(Bounds->Layer != NULL)
	{
		SrcX += Bounds->OffsetX - Args->DstX;
		SrcY += Bounds->OffsetY - Args->DstY;
	}

	/* Calculate position and size of the
	 * rectangle to fill in.
	 */
	DstX  = Bounds->Bounds.MinX;
	DstY  = Bounds->Bounds.MinY;
	SizeX = Bounds->Bounds.MaxX - Bounds->Bounds.MinX + 1;
	SizeY = Bounds->Bounds.MaxY - Bounds->Bounds.MinY + 1;

	BltBitMap( Args->SrcBitMap, SrcX,SrcY,RPort->BitMap,DstX,DstY,SizeX,SizeY,MINTERM_B_EQUALS_C,RPort->Mask,NULL);
	BltBitMap(&Args->MaskBitMap,SrcX,SrcY,RPort->BitMap,DstX,DstY,SizeX,SizeY,Args->MinTerm,     RPort->Mask,NULL);
	BltBitMap( Args->SrcBitMap, SrcX,SrcY,RPort->BitMap,DstX,DstY,SizeX,SizeY,MINTERM_B_EQUALS_C,RPort->Mask,NULL);
}

VOID
NewBltMaskBitMapRastPort(
	struct BitMap *		srcbm,
    WORD				srcx,
    WORD				srcy,
    struct RastPort *	destrp,
    WORD				destx,
    WORD				desty,
    WORD				sizex,
    WORD				sizey,
    UBYTE				minterm,
    PLANEPTR			bltmask,
    struct Library *	GfxBase,
    struct Library *	LayersBase)
{
	if( ! ( GetBitMapAttr( destrp->BitMap, BMA_FLAGS ) & BMF_INTERLEAVED ) )
    {
    	BltMaskBitMapRastPort(srcbm, srcx, srcy, destrp, destx,desty, sizex,sizey, minterm,bltmask);
    	return;
	}
		
	/* Valid parameters? */
	if(srcbm != NULL && destrp != NULL && bltmask != NULL && sizex > 0 && sizey > 0)
	{
		struct Rectangle Bounds;
		struct BltArgs Args;
		LONG Depth,i;

		/* Set up the hook and copy most parameters to
		 * temporary storage.
		 */
		Args.Hook.h_Entry	= (HOOKFUNC)FillRoutine;
		Args.Hook.h_Data	= &Args;
		Args.SrcBitMap		= srcbm;
		Args.SrcX			= srcx;
		Args.SrcY			= srcy;
		Args.DstX			= destx;
		Args.DstY			= desty;
		Args.MinTerm		= minterm;
		Args.GfxBase		= GfxBase;

		/* Initialize the mask bitmap. */
		Depth = GetBitMapAttr(srcbm,BMA_DEPTH);
		if(Depth > NUM_ELEMENTS(Args.MaskBitMap.Planes))
			Depth = NUM_ELEMENTS(Args.MaskBitMap.Planes);

		/* Set the mask bitmap up to match the size and
		 * dimensions of the source bitmap.
		 */
		bzero((char*)&Args.MaskBitMap,sizeof( struct BitMap ) );
		InitBitMap(&Args.MaskBitMap,Depth,
		                            GetBitMapAttr(srcbm,BMA_WIDTH),
		                            GetBitMapAttr(srcbm,BMA_HEIGHT));

		for(i = 0 ; i < Depth ; i++)
			Args.MaskBitMap.Planes[i] = bltmask;

		/* Restrict blitter operations to the given rectangle. */
		Bounds.MinX = destx;
		Bounds.MaxX = destx + sizex - 1;
		Bounds.MinY = desty;
		Bounds.MaxY = desty + sizey - 1;

		/* Do the blitter operation. */
		DoHookClipRects(&Args.Hook,destrp,&Bounds);		
	}
}

/****************************************************************************/