/***************************************************************************
 SimpleMail - Copyright (C) 2000 Hynek Schlawack and Sebastian Bauer

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
***************************************************************************/

/*
** datatypescache.c
*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/layers.h>

#include <dos.h>

#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

static struct List dt_list;
static int dt_initialized;

struct dt_node
{
    struct MinNode node;
    char *filename;
    Object *o;
    int width, height;
    struct Screen *scr;
    int count;
    struct BackFillInfo *bfi;
};

static struct MinNode *Node_Next(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ->mln_Succ == NULL)
		return NULL;
    return ((struct MinNode*)node)->mln_Succ;
}

static struct MinNode *Node_Prev(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Pred == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Pred->mln_Pred == NULL)
	return NULL;
    return ((struct MinNode*)node)->mln_Pred;
}

static struct MinNode *List_First(APTR list)
{
    if( !((struct MinList*)list)->mlh_Head) return NULL;
    if(((struct MinList*)list)->mlh_Head->mln_Succ == NULL) return NULL;
    return ((struct MinList*)list)->mlh_Head;
}

static struct MinNode *List_Last(APTR list)
{
    if( !((struct MinList*)list)->mlh_TailPred) return NULL;
    if(((struct MinList*)list)->mlh_TailPred->mln_Pred == NULL) return NULL;
    return ((struct MinList*)list)->mlh_TailPred;
}

/* A BltBitMaskPort() replacement which blits masks for interleaved bitmaps correctly */
struct LayerHookMsg
{
	struct Layer *layer;
	struct Rectangle bounds;
	LONG offsetx;
	LONG offsety;
};

struct BltMaskHook
{
  struct Hook hook;
  struct BitMap maskBitMap;
  struct BitMap *srcBitMap;
  LONG srcx,srcy;
  LONG destx,desty;
};

#ifndef _DCC
VOID MyBltMaskBitMap( CONST struct BitMap *srcBitMap, LONG xSrc, LONG ySrc, struct BitMap *destBitMap, LONG xDest, LONG yDest, LONG xSize, LONG ySize, struct BitMap *maskBitMap )
{
  BltBitMap(srcBitMap,xSrc,ySrc,destBitMap, xDest, yDest, xSize, ySize, 0x99,~0,NULL);
  BltBitMap(maskBitMap,xSrc,ySrc,destBitMap, xDest, yDest, xSize, ySize, 0xe2,~0,NULL);
  BltBitMap(srcBitMap,xSrc,ySrc,destBitMap, xDest, yDest, xSize, ySize, 0x99,~0,NULL);
}
#endif

__asm void HookFunc_BltMask(register __a0 struct Hook *hook, register __a1 struct LayerHookMsg *msg, register __a2 struct RastPort *rp )
{
  struct BltMaskHook *h = (struct BltMaskHook*)hook;

  LONG width = msg->bounds.MaxX - msg->bounds.MinX+1;
  LONG height = msg->bounds.MaxY - msg->bounds.MinY+1;
  LONG offsetx = h->srcx + msg->offsetx - h->destx;
  LONG offsety = h->srcy + msg->offsety - h->desty;

	putreg(REG_A4,(long)hook->h_Data);

  MyBltMaskBitMap( h->srcBitMap, offsetx, offsety, rp->BitMap, msg->bounds.MinX, msg->bounds.MinY, width, height, &h->maskBitMap);
}

VOID MyBltMaskBitMapRastPort( struct BitMap *srcBitMap, LONG xSrc, LONG ySrc, struct RastPort *destRP, LONG xDest, LONG yDest, LONG xSize, LONG ySize, ULONG minterm, APTR bltMask )
{
    if (GetBitMapAttr(srcBitMap,BMA_FLAGS)&BMF_INTERLEAVED)
    {
	LONG src_depth = GetBitMapAttr(srcBitMap,BMA_DEPTH);
	struct Rectangle rect;
	struct BltMaskHook hook;
		
	/* Define the destination rectangle in the rastport */
	rect.MinX = xDest;
	rect.MinY = yDest;
	rect.MaxX = xDest + xSize - 1;
	rect.MaxY = yDest + ySize - 1;
		
	/* Initialize the hook */
	hook.hook.h_Entry = (HOOKFUNC)HookFunc_BltMask;
	hook.hook.h_Data = (void*)getreg(REG_A4);
	hook.srcBitMap = srcBitMap;
	hook.srcx = xSrc;
	hook.srcy = ySrc;
	hook.destx = xDest;
	hook.desty = yDest;
		
	/* Initialize a bitmap where all plane pointers points to the mask */
	InitBitMap(&hook.maskBitMap,src_depth,GetBitMapAttr(srcBitMap,BMA_WIDTH),GetBitMapAttr(srcBitMap,BMA_HEIGHT));
	while (src_depth)
	    hook.maskBitMap.Planes[--src_depth] = bltMask;

	/* Blit onto the Rastport */
	DoHookClipRects(&hook.hook,destRP,&rect);
    } else
    {
	BltMaskBitMapRastPort(srcBitMap, xSrc, ySrc, destRP, xDest, yDest, xSize, ySize, minterm, bltMask);
    }
}


static Object *LoadPicture(char *filename, struct Screen *scr)
{
    Object *o;

    struct Process *myproc = (struct Process *)FindTask(NULL);
    APTR oldwindowptr = myproc->pr_WindowPtr;
    myproc->pr_WindowPtr = (APTR)-1;

    o = NewDTObject(filename,
	DTA_GroupID          , GID_PICTURE,
	OBP_Precision        , PRECISION_EXACT,
	PDTA_Screen          , scr,
	PDTA_FreeSourceBitMap, TRUE,
	PDTA_DestMode        , PMODE_V43,
	PDTA_UseFriendBitMap , TRUE,
	TAG_DONE);
	
    myproc->pr_WindowPtr = oldwindowptr;
	
    if (o)
    {
	struct FrameInfo fri = {0};
	DoMethod(o,DTM_FRAMEBOX,NULL,&fri,&fri,sizeof(struct FrameInfo),0);
	
	if (fri.fri_Dimensions.Depth>0)
	{
	    if (DoMethod(o,DTM_PROCLAYOUT,NULL,1))
	    {
		return o;
	    }
	}
	DisposeDTObject(o);
    }
    return NULL;
}

/*
void dt_init(void)
{
}

void dt_cleanup(void)
{
}
*/

struct dt_node *dt_load_picture(char *filename, struct Screen *scr)
{
    struct dt_node *node;
    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    if (!dt_initialized)
    {
	NewList(&dt_list);
	dt_initialized = 1;
    }

    node = (struct dt_node*)List_First(&dt_list);
    while (node)
    {
	if (!Stricmp(filename,node->filename))
	{
	    node->count++;
	    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
	    return node;
	}
	node = (struct dt_node*)Node_Next(node);
    }

    if ((node = (struct dt_node*)AllocVec(sizeof(struct dt_node),MEMF_CLEAR)))
    {
	/* create the datatypes object */
	if ((node->o = LoadPicture(filename,scr)))
	{
	    struct BitMapHeader *bmhd;
	    GetDTAttrs(node->o,PDTA_BitMapHeader,&bmhd,TAG_DONE);

	    if (bmhd)
	    {
		node->width = bmhd->bmh_Width;
		node->height = bmhd->bmh_Height;
	    }
	    node->scr = scr;

	    AddTail((struct List*)&dt_list,(struct Node*)node);
	    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
	    return node;
	}
	FreeVec(node);
    }
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    return NULL;
}

void dt_dispose_picture(struct dt_node *node)
{
    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    if (node && node->count)
    {
	node->count--;
	if (!node->count)
	{
	    Remove((struct Node*)node);
	    DisposeDTObject(node->o);
	    FreeVec(node);
	}
    }
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
}

int dt_width(struct dt_node *node)
{
    return node->width;
}

int dt_height(struct dt_node *node)
{
    return node->height;
}

void dt_put_on_rastport(struct dt_node *node, struct RastPort *rp, int x, int y)
{
    struct BitMap *bitmap = NULL;
    Object *o;

    o = node->o;
    if (!o) return;

    GetDTAttrs(o,PDTA_DestBitMap,&bitmap,TAG_DONE);
    if (!bitmap) GetDTAttrs(o,PDTA_BitMap,&bitmap,TAG_DONE);
    if (bitmap)
    {
	APTR mask = NULL;

	GetDTAttrs(o,PDTA_MaskPlane,&mask,TAG_DONE);
	if (mask)
	{
	    MyBltMaskBitMapRastPort(bitmap,0,0,rp,x,y,dt_width(node),dt_height(node),0xe2,(PLANEPTR)mask);
	} else BltBitMapRastPort(bitmap,0,0,rp,x,y,dt_width(node),dt_height(node),0xc0);
    }
}

#define RECTSIZEX(r) ((r)->MaxX-(r)->MinX+1)
#define RECTSIZEY(r) ((r)->MaxY-(r)->MinY+1)

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MOD(x,y) ((x)<0 ? (y)-((-(x))%(y)) : (x)%(y))

struct BackFillMsg
{
	struct Layer    *Layer;
	struct Rectangle Bounds;
	LONG             OffsetX;
	LONG             OffsetY;
};

struct BackFillOptions
{
	WORD MaxCopyWidth;  // maximum width for the copy
	WORD MaxCopyHeight; // maximum height for the copy
//	BOOL CenterX;       // center the tiles horizontally?
//	BOOL CenterY;       // center the tiles vertically?
	WORD OffsetX;       // offset to add
	WORD OffsetY;       // offset to add
	BOOL OffsetTitleY;  // add the screen titlebar height to the vertical offset?
};

struct BackFillInfo
{
	struct Hook   				 Hook;
	WORD									 Width;
	WORD							 	 Height;
	struct BitMap  			*BitMap;
/*	struct Screen				*Screen; */ /* Needed for centering */
	WORD            			 CopyWidth;
	WORD           	     CopyHeight;
	struct BackFillOptions Options;
	WORD									 OffsetX;
	WORD									 OffsetY;
};


static void CopyTiledBitMap(struct BitMap *Src,WORD SrcOffsetX,WORD SrcOffsetY,WORD SrcSizeX,WORD SrcSizeY,struct BitMap *Dst,struct Rectangle *DstBounds)
{
	WORD FirstSizeX;  // the width of the rectangle to blit as the first column
	WORD FirstSizeY;  // the height of the rectangle to blit as the first row
	WORD SecondMinX;  // the left edge of the second column
	WORD SecondMinY;  // the top edge of the second column
	WORD SecondSizeX; // the width of the second column
	WORD SecondSizeY; // the height of the second column
	WORD Pos;         // used as starting position in the "exponential" blit
	WORD Size;        // used as bitmap size in the "exponential" blit

	FirstSizeX = MIN(SrcSizeX-SrcOffsetX,RECTSIZEX(DstBounds)); // the width of the first tile, this is either the rest of the tile right to SrcOffsetX or the width of the dest rect, if the rect is narrow
	SecondMinX = DstBounds->MinX+FirstSizeX; // the start for the second tile (if used)
	SecondSizeX = MIN(SrcOffsetX,DstBounds->MaxX-SecondMinX+1); // the width of the second tile (we want the whole tile to be SrcSizeX pixels wide, if we use SrcSizeX-SrcOffsetX pixels for the left part we'll use SrcOffsetX for the right part)

	FirstSizeY = MIN(SrcSizeY-SrcOffsetY,RECTSIZEY(DstBounds)); // the same values are calculated for y direction
	SecondMinY = DstBounds->MinY+FirstSizeY;
	SecondSizeY = MIN(SrcOffsetY,DstBounds->MaxY-SecondMinY+1);

	BltBitMap(Src,SrcOffsetX,SrcOffsetY,Dst,DstBounds->MinX,DstBounds->MinY,FirstSizeX,FirstSizeY,0xC0,-1,NULL); // blit the first piece of the tile
	if (SecondSizeX>0) // if SrcOffset was 0 or the dest rect was to narrow, we won't need a second column
		BltBitMap(Src,0,SrcOffsetY,Dst,SecondMinX,DstBounds->MinY,SecondSizeX,FirstSizeY,0xC0,-1,NULL);
	if (SecondSizeY>0) // is a second row necessary?
	{
		BltBitMap(Src,SrcOffsetX,0,Dst,DstBounds->MinX,SecondMinY,FirstSizeX,SecondSizeY,0xC0,-1,NULL);
		if (SecondSizeX>0)
			BltBitMap(Src,0,0,Dst,SecondMinX,SecondMinY,SecondSizeX,SecondSizeY,0xC0,-1,NULL);
	}

	// this loop generates the first row of the tiles
	for (Pos = DstBounds->MinX+SrcSizeX,Size = MIN(SrcSizeX,DstBounds->MaxX-Pos+1);Pos<=DstBounds->MaxX;)
	{
		BltBitMap(Dst,DstBounds->MinX,DstBounds->MinY,Dst,Pos,DstBounds->MinY,Size,MIN(SrcSizeY,RECTSIZEY(DstBounds)),0xC0,-1,NULL);
		Pos += Size;
		Size = MIN(Size<<1,DstBounds->MaxX-Pos+1);
	}

	// this loop blit the first row down several times to fill the whole dest rect
	for (Pos = DstBounds->MinY+SrcSizeY,Size = MIN(SrcSizeY,DstBounds->MaxY-Pos+1);Pos<=DstBounds->MaxY;)
	{
		BltBitMap(Dst,DstBounds->MinX,DstBounds->MinY,Dst,DstBounds->MinX,Pos,RECTSIZEX(DstBounds),Size,0xC0,-1,NULL);
		Pos += Size;
		Size = MIN(Size<<1,DstBounds->MaxY-Pos+1);
	}
}

__asm STATIC void WindowPatternBackFillFunc(register __a0 struct Hook *Hook,register __a2 struct RastPort *RP,register __a1 struct BackFillMsg *BFM)
{
	WORD OffsetX; // the offset within the tile in x direction
	WORD OffsetY; // the offset within the tile in y direction

	struct BackFillInfo *BFI = (struct BackFillInfo *)Hook; // get the data for our backfillhook (but __saveds is nonetheless required because this function needs GfxBase)

#ifndef __MAXON__
	putreg(12,(long)Hook->h_Data);
#endif

	OffsetX = BFM->Bounds.MinX-BFI->Options.OffsetX; // The first tile normally isn't totally visible => calculate the offset (offset 0 would mean that the left edge of the damage rectangle coincides with the left edge of a tile)
//	if (BFI->Options.CenterX) // horizontal centering?
//		OffsetX -= (BFI->Screen->Width-BFI->Width)/2;

	OffsetY = BFM->Bounds.MinY-BFI->Options.OffsetY; // The same values are calculated for y direction

/*
	if (BFI->Options.OffsetTitleY) // shift the tiles down?
		OffsetY -= BFI->Screen->BarHeight+1;
*/

//	if (BFI->Options.CenterY) // horizontal centering?
//		OffsetY -= (BFI->Screen->Height - BFI->Height)/2;

	CopyTiledBitMap(BFI->BitMap,MOD(OffsetX+BFI->OffsetX,BFI->Width),MOD(OffsetY+BFI->OffsetY,BFI->Height),BFI->CopyWidth,BFI->CopyHeight,RP->BitMap,&BFM->Bounds);
}

static void CalculateCopySizes(struct BackFillInfo *BFI)
{
	BFI->CopyWidth = (BFI->Width>BFI->Options.MaxCopyWidth) ? BFI->Width : BFI->Options.MaxCopyWidth-BFI->Options.MaxCopyWidth%BFI->Width;
	BFI->CopyHeight = (BFI->Height>BFI->Options.MaxCopyHeight) ? BFI->Height : BFI->Options.MaxCopyHeight-BFI->Options.MaxCopyHeight%BFI->Height;
}
/*
**********************************************************/

void dt_put_on_rastport_tiled(struct dt_node *node, struct RastPort *rp, int x1, int y1, int x2, int y2, int xoffset, int yoffset)
{
    struct Screen *scr = node->scr;
    struct BitMap *bitmap;
    Object *o;

    o = node->o;
    if (!o) return;

    GetDTAttrs(o,PDTA_DestBitMap,&bitmap,TAG_DONE);
    if (!bitmap) GetDTAttrs(o,PDTA_BitMap,&bitmap,TAG_DONE);
    if (!bitmap) return;

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    if (!node->bfi)
    {
	struct BackFillInfo *bfi = (struct BackFillInfo*)AllocVec(sizeof(struct BackFillInfo),MEMF_CLEAR);
	if (bfi)
	{
	    LONG depth = GetBitMapAttr(bitmap,BMA_DEPTH);
	    bfi->Hook.h_Entry = (ULONG (*)())WindowPatternBackFillFunc;
#ifndef __MAXON__
	    bfi->Hook.h_Data = (APTR)getreg(12);	/* register A4 */
#endif

	    bfi->Options.MaxCopyWidth = 256;
	    bfi->Options.MaxCopyHeight = 256;
//	    bfi->Options.CenterX = FALSE;				/* center the tiles horizontally? */
//	    bfi->Options.CenterY = FALSE;				/* center the tiles vertically? */
	    bfi->Options.OffsetX = 0;						/* offset to add */
	    bfi->Options.OffsetY = 0;						/* offset to add */
	    bfi->Options.OffsetTitleY = TRUE;		/* add the screen titlebar height to the vertical offset? */
	    bfi->Width = dt_width(node);
	    bfi->Height = dt_height(node);

	    CalculateCopySizes(bfi);

	    if((bfi->BitMap = AllocBitMap(bfi->CopyWidth,bfi->CopyHeight,depth, BMF_INTERLEAVED|BMF_MINPLANES,scr->RastPort.BitMap)))
	    {
		struct Rectangle CopyBounds;
		CopyBounds.MinX = 0;
		CopyBounds.MinY = 0;
		CopyBounds.MaxX = bfi->CopyWidth-1;
		CopyBounds.MaxY = bfi->CopyHeight-1;

		CopyTiledBitMap(bitmap,0,0,bfi->Width,bfi->Height,bfi->BitMap,&CopyBounds);
	    }
	}
	node->bfi = bfi;
    }

    if (node->bfi)
    {
	struct BackFillInfo *bfi = node->bfi;
	struct Rectangle rect;

	rect.MinX = x1;
	rect.MinY = y1;
	rect.MaxX = x2;
	rect.MaxY = y2;

	if (rp->Layer)
	{
	    xoffset -= rp->Layer->bounds.MinX;
	    yoffset -= rp->Layer->bounds.MinY;
	}

	bfi->OffsetX = xoffset;
	bfi->OffsetY = yoffset;

	DoHookClipRects((struct Hook*)bfi,rp,&rect);
    }
    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
}

