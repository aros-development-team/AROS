/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <exec/devices.h>
#include <exec/memory.h>
#include <workbench/icon.h>

#include <clib/alib_protos.h>

#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/timer.h>
#include <proto/utility.h>

#include "dragndrop.h"
#include "muimaster_intern.h"
#include "support.h"

/* #define MYDEBUG 1 */
#include "debug.h"

extern struct Library *MUIMasterBase;

#ifdef __MAXON__

ULONG IconControl( struct DiskObject *icon, ... )
{
	return IconControlA(icon,(struct TagItem*)((((ULONG*)&icon)+1)));
}

struct DiskObject *GetIconTags( CONST_STRPTR name, ... )
{
	return GetIconTagList(name,(struct TagItem*)(((ULONG*)&name)+1));
}


#define ASM

#else

#ifdef __SASC
#define ASM __asm
#else
#define ASM
#endif

#endif

//-------------------------------------
// List Funcs
//-------------------------------------
static struct MinNode *Node_Prev(APTR node)
{
	if(node == NULL) return NULL;
	if(((struct MinNode*)node)->mln_Pred == NULL) return NULL;
	if(((struct MinNode*)node)->mln_Pred->mln_Pred == NULL)
		return NULL;
	return ((struct MinNode*)node)->mln_Pred;
}
//-------------------------------------

//-------------------------------------
static struct MinNode *List_Last(APTR list)
{
	if( !((struct MinList*)list)->mlh_TailPred) return NULL;

	if(((struct MinList*)list)->mlh_TailPred->mln_Pred == NULL) return NULL;
	return ((struct MinList*)list)->mlh_TailPred;
}
//-------------------------------------
#if 0
static ULONG List_Length(APTR list)
{
	struct MinNode *node = List_First(list);
	ULONG len=0;
	while(node)
	{
		len++;
		node = Node_Next(node);
	}
	return len;
}
//-------------------------------------
static struct MinNode *List_Find(APTR list, ULONG num)
{
	struct MinNode *node = List_First(list);
	while(num--)
	{
		if(!(node = Node_Next(node))) break;
	}
	return node;
}
#endif
//-------------------------------------

struct DragNDrop
{
	struct MinList dnd_List;
	struct Screen *dnd_Screen;
	struct BitMap *dnd_TempBitMap;

/*
	struct RastPort dnd_RastPort;
	struct Layer_Info *dnd_LayerInfo;
	struct Layer *dnd_Layer;
*/
};

struct BitMapNode
{
	struct MinNode bmn_Node;
	struct BitMap *bmn_BitMap;
	APTR bmn_Mask;

	LONG bmn_Left;
	LONG bmn_Top;
	LONG bmn_Width;
	LONG bmn_Height;

	LONG bmn_SaveX;
	LONG bmn_SaveY;
	LONG bmn_SaveWidth;
	LONG bmn_SaveHeight;
	LONG bmn_SaveOffX;
	LONG bmn_SaveOffY;
	LONG bmn_Drawed;
	struct BitMap *bmn_SaveBitMap;

	struct DragNDrop *bmn_DnD;
};

#define bmn_Succ bmn_Node.mln_Succ
#define bmn_Pred bmn_Node.mln_Pred

/* Tags for GUI_CreateBitMapNodeA() */
#define GUI_BitMap				(TAG_USER+1)	/* struct BitMap * */
#define GUI_Mask					(TAG_USER+2)	/* APTR */
#define GUI_LeftOffset		(TAG_USER+3)	/* LONG */
#define GUI_TopOffset		(TAG_USER+4)	/* LONG */
#define GUI_Width				(TAG_USER+5)	/* LONG */
#define GUI_Height				(TAG_USER+6)	/* LONG */

//-------------------------------------
STATIC VOID List_Sort_Mode_1( struct MinList *list )
{
	BOOL notfinished=TRUE;

	/* Sort list (quick & dirty bubble sort) */
	while( notfinished )
	{
		struct BitMapNode *first;

		/* Reset not finished flag */
		notfinished = FALSE;

		/* Get first node */
		if(( first = List_First(list)))
		{
			struct BitMapNode *second;

			/* One bubble sort round */
			while(( second = Node_Next(first)))
			{
				BOOL sort;
				if(first->bmn_Top > second->bmn_Top) sort=TRUE;
				else if(first->bmn_Top == second->bmn_Top && first->bmn_Left > second->bmn_Left) sort = TRUE;
				else sort=FALSE;

				if( sort )
				{
					Remove((struct Node*)first);
					Insert((struct List*)list,(struct Node*)first,(struct Node*)second);
					notfinished=TRUE;
				}	else first=second;
			}
		}
	}
}
//-------------------------------------
#if 0
STATIC VOID List_Sort_Mode_2( struct MinList *list )
{
	BOOL notfinished=TRUE;

	/* Sort list (quick & dirty bubble sort) */
	while( notfinished )
	{
		struct BitMapNode *first;

		/* Reset not finished flag */
		notfinished = FALSE;

		/* Get first node */
		if(( first = List_First(list)))
		{
			struct BitMapNode *second;

			/* One bubble sort round */
			while(( second = Node_Next(first)))
			{
				BOOL sort;
				if(first->bmn_Top > second->bmn_Top) sort=TRUE;
				else if(first->bmn_Top == second->bmn_Top && first->bmn_Left < second->bmn_Left) sort = TRUE;
				else sort=FALSE;

				if( sort )
				{
					Remove((struct Node*)first);
					Insert((struct List*)list,(struct Node*)first,(struct Node*)second);
					notfinished=TRUE;
				}	else first=second;
			}
		}
	}
}
#endif
//-------------------------------------
STATIC VOID List_Sort_Mode_3( struct MinList *list )
{
	BOOL notfinished=TRUE;

	/* Sort list (quick & dirty bubble sort) */
	while( notfinished )
	{
		struct BitMapNode *first;

		/* Reset not finished flag */
		notfinished = FALSE;

		/* Get first node */
		if(( first = List_First(list)))
		{
			struct BitMapNode *second;

			/* One bubble sort round */
			while(( second = Node_Next(first)))
			{
				BOOL sort;
				if(first->bmn_Left > second->bmn_Left) sort=TRUE;
				else if(first->bmn_Left == second->bmn_Left && first->bmn_Top > second->bmn_Top) sort = TRUE;
				else sort=FALSE;

				if( sort )
				{
					Remove((struct Node*)first);
					Insert((struct List*)list,(struct Node*)first,(struct Node*)second);
					notfinished=TRUE;
				}	else first=second;
			}
		}
	}
}
//-------------------------------------
STATIC BOOL AndRectangle( struct Rectangle *a, struct Rectangle *b, struct Rectangle *c)
{
	c->MinX = MAX(a->MinX,b->MinX);
	c->MinY = MAX(a->MinY,b->MinY);
	c->MaxX = MIN(a->MaxX,b->MaxX);
	c->MaxY = MIN(a->MaxY,b->MaxY);

	if((c->MinX > c->MaxX) || (c->MinY > c->MaxY))  return FALSE;

	return TRUE;
}
//-------------------------------------

//-------------------------------------
STATIC VOID SafeBltBitMapRastPort( struct BitMap *srcBitMap, long xSrc, long ySrc,
	struct RastPort *destRP, long xDest, long yDest, long xSize,
	long ySize, unsigned long minterm )
{
	struct BitMap *destBitMap = destRP->BitMap;
	LONG srcMaxWidth, srcMaxHeight;
	LONG destMaxWidth, destMaxHeight;

	srcMaxWidth = GetBitMapAttr(srcBitMap, BMA_WIDTH);
	srcMaxHeight = GetBitMapAttr(srcBitMap, BMA_HEIGHT);
	destMaxWidth = GetBitMapAttr(destBitMap, BMA_WIDTH);
	destMaxHeight = GetBitMapAttr(destBitMap, BMA_HEIGHT);

	if(xSrc<0)
	{
		xDest -= xSrc;
		xSize += xSrc;
		xSrc=0;
	}

	if(ySrc<0)
	{
		yDest -= ySrc;
		ySize += ySrc;
		ySrc=0;
	}

	if(xDest<0)
	{
		xSrc -= xDest;
		xSize += xDest;
		xDest = 0;
	}

	if(yDest<0)
	{
		ySrc -= yDest;
		ySize += yDest;
		yDest = 0;
	}

	if( xSize + xSrc > srcMaxWidth ) xSize = srcMaxWidth-xSrc;
	if( ySize + ySrc > srcMaxHeight ) ySize = srcMaxHeight-ySrc;
	if( xSize + xDest > destMaxWidth ) xSize = destMaxWidth-xDest;
	if( ySize + yDest > destMaxHeight ) ySize = destMaxHeight-yDest;

	if(xSize > 0 && ySize > 0)
	{
		BltBitMapRastPort(srcBitMap,xSrc,ySrc,destRP,xDest,yDest,xSize,ySize,minterm);
	}
}
//-------------------------------------
STATIC LONG SafeBltBitMap( struct BitMap *srcBitMap, long xSrc, long ySrc,
	struct BitMap *destBitMap, long xDest, long yDest, long xSize,
	long ySize, unsigned long minterm, unsigned long mask,
	PLANEPTR tempA )
{
	LONG srcMaxWidth, srcMaxHeight;
	LONG destMaxWidth, destMaxHeight;

	srcMaxWidth = GetBitMapAttr(srcBitMap, BMA_WIDTH);
	srcMaxHeight = GetBitMapAttr(srcBitMap, BMA_HEIGHT);
	destMaxWidth = GetBitMapAttr(destBitMap, BMA_WIDTH);
	destMaxHeight = GetBitMapAttr(destBitMap, BMA_HEIGHT);

	if(xSrc<0)
	{
		xDest -= xSrc;
		xSize += xSrc;
		xSrc=0;
	}

	if(ySrc<0)
	{
		yDest -= ySrc;
		ySize += ySrc;
		ySrc=0;
	}

	if(xDest<0)
	{
		xSrc -= xDest;
		xSize += xDest;
		xDest = 0;
	}

	if(yDest<0)
	{
		ySrc -= yDest;
		ySize += yDest;
		yDest = 0;
	}

	if( xSize + xSrc > srcMaxWidth ) xSize = srcMaxWidth-xSrc;
	if( ySize + ySrc > srcMaxHeight ) ySize = srcMaxHeight-ySrc;
	if( xSize + xDest > destMaxWidth ) xSize = destMaxWidth-xDest;
	if( ySize + yDest > destMaxHeight ) ySize = destMaxHeight-yDest;

	if(xSize > 0 && ySize > 0)
	{
		return BltBitMap( srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize, minterm, mask, tempA );
	}
	return 0;
}

//-------------------------------------
STATIC VOID BltBackgroundBitMap( struct BitMapNode *dest_bmn, long xSrc, long ySrc, long xSize, long ySize, ULONG use_temp)
{
	struct BitMap *srcBitMap,*destBitMap;
	struct DragNDrop *dnd = dest_bmn->bmn_DnD;
	LONG maxWidth, maxHeight;
	LONG xDest = 0, yDest = 0;

	struct Rectangle rect;
	rect.MinX = xSrc;
	rect.MinY = ySrc;
	rect.MaxX = xSrc + xSize - 1;
	rect.MaxY = ySrc + ySize - 1;

	srcBitMap = dnd->dnd_Screen->RastPort.BitMap;

	if(use_temp) destBitMap = dnd->dnd_TempBitMap;
	else destBitMap = dest_bmn->bmn_SaveBitMap;

	maxWidth = GetBitMapAttr(srcBitMap, BMA_WIDTH);
	maxHeight = GetBitMapAttr(srcBitMap, BMA_HEIGHT);

	if(xSrc<0)
	{
		xDest -= xSrc;
		xSize += xSrc;
		xSrc=0;
	}

	if(ySrc<0)
	{
		yDest -= ySrc;
		ySize += ySrc;
		ySrc=0;
	}

	if( xSize + xSrc > maxWidth ) xSize = maxWidth-xSrc;
	if( ySize + ySrc > maxHeight ) ySize = maxHeight-ySrc;

	if(xSize > 0 && ySize > 0)
	{
		struct BitMapNode *bmn = List_First(&dnd->dnd_List);

		SafeBltBitMap(srcBitMap,xSrc,ySrc,destBitMap,xDest,yDest,xSize,ySize,0xc0, -1, NULL);


//		BltBitMapRastPort(destBitMap,0,0,
//						&dnd->dnd_Screen->RastPort, 2*dest_bmn->bmn_Left+150, dest_bmn->bmn_Top+200, xSize, ySize,0xc0);

		while(bmn)
		{
			if(bmn != dest_bmn)
			{
				struct Rectangle bmn_rect,result_rect;
				bmn_rect.MinX = bmn->bmn_SaveX;
				bmn_rect.MinY = bmn->bmn_SaveY;
				bmn_rect.MaxX = bmn_rect.MinX + bmn->bmn_SaveWidth - 1;
				bmn_rect.MaxY = bmn_rect.MinY + bmn->bmn_SaveHeight - 1;

				if(AndRectangle(&rect,&bmn_rect,&result_rect))
				{
					LONG bmn_x = result_rect.MinX - bmn_rect.MinX;
					LONG bmn_y = result_rect.MinY - bmn_rect.MinY;
					LONG bmn_width = result_rect.MaxX - result_rect.MinX + 1;
					LONG bmn_height = result_rect.MaxY - result_rect.MinY + 1;
					LONG xDest = result_rect.MinX - rect.MinX;
					LONG yDest = result_rect.MinY - rect.MinY;

					SafeBltBitMap(bmn->bmn_SaveBitMap, bmn_x, bmn_y,
												destBitMap, xDest, yDest, bmn_width, bmn_height,0xc0,-1,NULL);

//		BltBitMapRastPort(destBitMap,0,0,
//						&dnd->dnd_Screen->RastPort, 100,300, xSize, ySize,0xc0);

				}
			}
			bmn = Node_Next(bmn);
		}

//		BltBitMapRastPort(destBitMap,0,0,
//						&dnd->dnd_Screen->RastPort, 2*dest_bmn->bmn_Left+150, dest_bmn->bmn_Top+200, xSize, ySize,0xc0);

	}
}
//-------------------------------------
STATIC VOID BltBitMapNode(struct BitMapNode *src_bmn, LONG offx, LONG offy, struct RastPort *rp, LONG x, LONG y, LONG width, LONG height)
{
	struct BitMap *destBitMap = rp->BitMap;
	LONG destMaxWidth = GetBitMapAttr(destBitMap, BMA_WIDTH);
	LONG destMaxHeight = GetBitMapAttr(destBitMap, BMA_HEIGHT);

	if(x<0)
	{
		offx -= x;
		width += x;
		x = 0;
	}

	if(y<0)
	{
		offy -= y;
		height += y;
		y = 0;
	}

	if( width + x > destMaxWidth ) width = destMaxWidth - x;
	if( height + y > destMaxHeight ) height = destMaxHeight - y;

	if(width > 0 && height > 0)
	{
		if(src_bmn->bmn_Mask)
		{
			BltMaskBitMapRastPort( src_bmn->bmn_BitMap, offx, offy,
									rp, x, y, width, height, 0xe2, (PLANEPTR)src_bmn->bmn_Mask);
		}	else
		{
			BltBitMapRastPort( src_bmn->bmn_BitMap, offx, offy,
									rp, x, y, width, height, 0xc0 );
		}
	}
}
//-------------------------------------
STATIC VOID BltNearBitMaps(struct BitMapNode *src_bmn, struct RastPort *rp,  LONG x, LONG y, LONG width, LONG height)
{
	struct DragNDrop *dnd = src_bmn->bmn_DnD;
	struct BitMapNode *bmn = List_First(&dnd->dnd_List);
	struct Rectangle rect;

	rect.MinX = x;
	rect.MinY = y;
	rect.MaxX = x + width - 1;
	rect.MaxY = y + height - 1;

	while(bmn)
	{
		if(bmn != src_bmn && bmn->bmn_Drawed)
		{
			struct Rectangle bmn_rect,result_rect;
			bmn_rect.MinX = bmn->bmn_SaveX;
			bmn_rect.MinY = bmn->bmn_SaveY;
			bmn_rect.MaxX = bmn_rect.MinX + bmn->bmn_SaveWidth - 1;
			bmn_rect.MaxY = bmn_rect.MinY + bmn->bmn_SaveHeight - 1;

			if(AndRectangle(&rect,&bmn_rect,&result_rect))
			{
				LONG bmn_x = result_rect.MinX - bmn_rect.MinX;
				LONG bmn_y = result_rect.MinY - bmn_rect.MinY;
				LONG bmn_width = result_rect.MaxX - result_rect.MinX + 1;
				LONG bmn_height = result_rect.MaxY - result_rect.MinY + 1;
				LONG xDest = result_rect.MinX - rect.MinX;
				LONG yDest = result_rect.MinY - rect.MinY;

				BltBitMapNode(bmn, bmn_x, bmn_y,
										rp, xDest, yDest, bmn_width, bmn_height);
				
			}
		}
		bmn = Node_Next(bmn);
	}
}
//-------------------------------------
STATIC VOID RestoreBackground( struct BitMapNode *src_bmn, struct RastPort *rp)
{
	LONG save_x = src_bmn->bmn_SaveX;
	LONG save_y = src_bmn->bmn_SaveY;
	LONG save_width = src_bmn->bmn_SaveWidth;
	LONG save_height = src_bmn->bmn_SaveHeight;

	struct DragNDrop *dnd = src_bmn->bmn_DnD;
	struct BitMapNode *bmn = List_First(&dnd->dnd_List);
	struct Rectangle last_rect;

	last_rect.MinX = save_x;
	last_rect.MinY = save_y;
	last_rect.MaxX = save_x + save_width - 1;
	last_rect.MaxY = save_y + save_height - 1;

	SafeBltBitMapRastPort( src_bmn->bmn_SaveBitMap,0,0,
							rp, save_x, save_y, save_width, save_height, 0xc0 );


	while(bmn)
	{
		if(bmn != src_bmn && bmn->bmn_Drawed)
		{
			struct Rectangle bmn_rect,result_rect;
			bmn_rect.MinX = bmn->bmn_SaveX;
			bmn_rect.MinY = bmn->bmn_SaveY;
			bmn_rect.MaxX = bmn_rect.MinX + bmn->bmn_SaveWidth - 1;
			bmn_rect.MaxY = bmn_rect.MinY + bmn->bmn_SaveHeight - 1;

			if(AndRectangle(&last_rect,&bmn_rect,&result_rect))
			{
				LONG bmn_x = result_rect.MinX - bmn_rect.MinX;
				LONG bmn_y = result_rect.MinY - bmn_rect.MinY;
				LONG bmn_width = result_rect.MaxX - result_rect.MinX + 1;
				LONG bmn_height = result_rect.MaxY - result_rect.MinY + 1;
/*  				LONG xDest = result_rect.MinX - last_rect.MinX; */
/*  				LONG yDest = result_rect.MinY - last_rect.MinY; */

				BltBitMapNode(bmn, bmn_x, bmn_y,
										rp, result_rect.MinX, result_rect.MinY, bmn_width, bmn_height);

//				SafeBltBitMapRastPort(bmn->bmn_SaveBitMap, bmn_x, bmn_y,
//											rp, xDest, yDest, bmn_width, bmn_height,0xc0);
				
			}
		}
		bmn = Node_Next(bmn);
	}
}
//-------------------------------------
struct BitMapNode *CreateBitMapNodeA( struct TagItem *tagList )
{
	struct BitMapNode *bmn = (struct BitMapNode*)AllocMem( sizeof(struct BitMapNode), MEMF_CLEAR );
	if( bmn )
	{
/*  		BOOL alloc=FALSE; */
		struct TagItem *tl=tagList;
		struct TagItem *tag;

		while(( tag = NextTagItem((const struct TagItem **) &tl )))
		{
			ULONG id = tag->ti_Tag;
			ULONG data = tag->ti_Data;

			switch( id )
			{
				case	GUI_BitMap:
							bmn->bmn_BitMap = (struct BitMap *)data;
							break;

				case	GUI_Mask:
							bmn->bmn_Mask = (APTR)data;
							break;

				case	GUI_LeftOffset:
							bmn->bmn_Left = (LONG)data;
							break;

				case	GUI_TopOffset:
							bmn->bmn_Top = (LONG)data;
							break;

				case	GUI_Width:
							bmn->bmn_Width = (LONG)data;
							break;

				case	GUI_Height:
							bmn->bmn_Height = (LONG)data;
							break;

			}
		}

		if( !bmn->bmn_BitMap )
		{
			FreeMem(bmn, sizeof(struct BitMapNode));
			bmn = NULL;
		}
	}
	return bmn;
}
//-------------------------------------
VOID DeleteBitMapNode(struct BitMapNode *bmn )
{
	if( bmn->bmn_SaveBitMap ) FreeBitMap(bmn->bmn_SaveBitMap);
	FreeMem( bmn, sizeof(struct BitMapNode));
}
//-------------------------------------
struct BitMap *GetBitMap( struct BitMapNode *bmn )
{
	if( bmn ) return bmn->bmn_BitMap;
	return NULL;
}
//-------------------------------------
VOID AttachBitMapNode( struct DragNDrop *dnd, struct BitMapNode *bmn )
{
	AddTail( (struct List*)&dnd->dnd_List, (struct Node*)&bmn->bmn_Node );
	bmn->bmn_DnD = dnd;
}
//-------------------------------------
VOID DetachBitMapNode( struct BitMapNode *bmn )
{
	if( bmn->bmn_Succ && bmn->bmn_Pred )
	{
		Remove((struct Node *)&bmn->bmn_Node );
		bmn->bmn_Succ = bmn->bmn_Pred = NULL;
	}
	bmn->bmn_DnD = NULL;
}
//-------------------------------------
VOID DrawBitMapNode( struct BitMapNode *bmn, LONG x, LONG y )
{
	LONG width = bmn->bmn_Width;
	LONG height = bmn->bmn_Height;
	LONG save_x = bmn->bmn_SaveX;
	LONG save_y = bmn->bmn_SaveY;
	LONG save_width = bmn->bmn_SaveWidth;
	LONG save_height = bmn->bmn_SaveHeight;
	struct RastPort *rp;
	struct BitMap *temp_bmap;
	BOOL draw=TRUE;//FALSE;

	if(!bmn || !bmn->bmn_DnD || !bmn->bmn_DnD->dnd_Screen) return;
	rp = &bmn->bmn_DnD->dnd_Screen->RastPort;
	temp_bmap = bmn->bmn_DnD->dnd_TempBitMap;

	if( !bmn->bmn_SaveBitMap ) return;

/*	if( bmn->bmn_SaveWidth > 0 && bmn->bmn_SaveHeight > 0 )
	{
		if(!temp_bmap)
		{
			RestoreBackground(bmn,rp);
		}
	}*/

	{
		LONG maxWidth, maxHeight/*  , offx=0, offy=0, save_offx=0, save_offy=0 */;
		LONG real_width = width, real_height = height;
		LONG real_save_width = save_width, real_save_height = save_height;

		maxWidth = GetBitMapAttr(rp->BitMap, BMA_WIDTH);
		maxHeight = GetBitMapAttr(rp->BitMap, BMA_HEIGHT);

		if( x < 0 ) real_width += x;
		if( y < 0 ) real_height += y;
		if( save_x < 0 ) real_save_width += save_x;
		if( save_y < 0 ) real_save_height += save_y;

		if( real_width + x > maxWidth ) real_width = maxWidth-x;
		if( real_height + y > maxHeight ) real_height = maxHeight-y;
		if( real_save_width + x > maxWidth ) real_save_width = maxWidth-x;
		if( real_save_height + y > maxHeight ) real_save_height = maxHeight-y;

		if((real_width>0 && real_height > 0)||(real_save_width>0&&real_save_height>0))
			draw = TRUE;
	}

	if( draw )
	{
		if(!temp_bmap)
		{
//			SafeBltBitMap(rp->BitMap, x,y,
//								bmn->bmn_SaveBitMap, 0,0, width, height, 0xc0, -1, NULL);

//			bmn->bmn_SaveWidth = 0;
//			bmn->bmn_SaveHeight = 0;

//			BltBackgroundBitMap(bmn->bmn_DnD, x, y,
//									bmn->bmn_SaveBitMap, 0,0, width, height);

			if( bmn->bmn_SaveWidth > 0 && bmn->bmn_SaveHeight > 0 )
				RestoreBackground(bmn,rp);

			BltBackgroundBitMap(bmn, x, y, width, height,FALSE);

//			BltBitMapRastPort(bmn->bmn_SaveBitMap,0,0,
//								rp, 20+bmn->bmn_Left*2,20+bmn->bmn_Top,width,height,0xc0);


			BltBitMapNode(bmn, 0,0, rp,x,y,width,height);
		}	else
		{
			struct RastPort temp_rp;
			struct Rectangle save_rect,rect,result_rect;
			InitRastPort(&temp_rp);
			temp_rp.BitMap = temp_bmap;

			save_rect.MinX = save_x;
			save_rect.MinY = save_y;
			save_rect.MaxX = save_x + save_width - 1;
			save_rect.MaxY = save_y + save_height - 1;

			rect.MinX = x;
			rect.MinY = y;
			rect.MaxX = x + width-1;
			rect.MaxY = y + height-1;

			if(AndRectangle(&rect,&save_rect,&result_rect))
			{
				LONG result_width = result_rect.MaxX - result_rect.MinX + 1;
				LONG result_height = result_rect.MaxY - result_rect.MinY + 1;
				LONG result_x = result_rect.MinX - save_rect.MinX;
				LONG result_y = result_rect.MinY - save_rect.MinY;
//				cout << rect.MinX << "  " << rect.MaxX << "  " << rect.MinY << "  " << rect.MaxY << endl;
//				cout << save_rect.MinX << "  " << save_rect.MaxX << "  " << save_rect.MinY << "  " << save_rect.MaxY << endl;
//				cout << result_rect.MinX << "  " << result_rect.MaxX << "  " << result_rect.MinY << "  " << result_rect.MaxY << endl;
//				cout << "soffx:" << save_offx << " offx:" << offx << " rx:" << result_x << "  " << result_y << "  " << "  w: " << width << "  " << result_width << "  " << result_height << endl;

//				SetRast(&temp_rp,0);

				// Neuen Hintergrund in temporäre Bitmap
//				SafeBltBitMapRastPort( rp->BitMap, x, y,
//										&temp_rp, 0, 0, width, height,0xc0);

				BltBackgroundBitMap(bmn, x, y, width, height, TRUE);


//		BltBitMapRastPort(temp_bmap,0,0,
//								rp, 100+bmn->bmn_Left,20+bmn->bmn_Top,bmn->bmn_Width,bmn->bmn_Height,0xc0);

				// Teile des alten Hintergrundes, die neu verdeckt werden in temporäre Bitmap
				BltBitMapRastPort( bmn->bmn_SaveBitMap, result_x, result_y,
										&temp_rp, (result_x?0:(save_width-result_width)),result_y?0:(save_height-result_height),
										result_width,result_height,0xc0);


//		BltBitMapRastPort(temp_bmap,0,0,
//								rp, 180+bmn->bmn_Left,20+bmn->bmn_Top,bmn->bmn_Width,bmn->bmn_Height,0xc0);


				// Teile des alten Hintergrundes, die nicht mehr verdeckt werden auf Screen
				if((save_width - result_width)>0)
				{
					SafeBltBitMapRastPort( bmn->bmn_SaveBitMap, (result_x?0:(result_width)),0,
										rp,save_x+(result_x?0:(result_width)),save_y,
										save_width-result_width,save_height,0xc0);
				}

				if((save_height - result_height)>0)
				{
					SafeBltBitMapRastPort( bmn->bmn_SaveBitMap, 0,result_y?0:(result_height),
										rp,save_x,save_y+(result_y?0:(result_height)),
										save_width,save_height-result_height,0xc0);
				}

				// temporäre BitMap ist neuer Hintergrund
				BltBitMap(temp_bmap,0,0,
								bmn->bmn_SaveBitMap, 0,0,width,height,0xc0,-1,NULL);

				// darzustellende BitMap in temporäre BitMap
				BltBitMapNode(bmn, 0,0,&temp_rp,0,0,width,height);

				// Angenzende BitMaps in temporäre BitMap
				BltNearBitMaps(bmn, &temp_rp,x,y,width,height);

//		BltBitMapRastPort(temp_bmap,0,0,
//								rp, 240+bmn->bmn_Left,20+bmn->bmn_Top,width,height,0xc0);


				// temporäre (fertige) BitMap darstellen
				SafeBltBitMapRastPort(temp_bmap,0,0,
								rp,x,y,width,height,0xc0);

//		BltBitMapRastPort(bmn->bmn_SaveBitMap,0,0,
//								rp, 40+bmn->bmn_Left,20+bmn->bmn_Top,bmn->bmn_Width,bmn->bmn_Height,0xc0);

			}	else
			{
				if( bmn->bmn_SaveWidth > 0 && bmn->bmn_SaveHeight > 0 )
					RestoreBackground(bmn,rp);

				BltBackgroundBitMap(bmn, x, y, width, height,FALSE);

/*				BltBitMapRastPort( bmn->bmn_SaveBitMap,0,0,
									rp, bmn->bmn_SaveX, bmn->bmn_SaveY, bmn->bmn_SaveWidth, bmn->bmn_SaveHeight, 0xc0 );

				SafeBltBitMap(rp->BitMap, x,y,
								bmn->bmn_SaveBitMap, 0,0, width, height, 0xc0, -1, NULL);
*/
				BltBitMapNode(bmn, 0,0,rp,x,y,width,height);
			}
			DeinitRastPort(&temp_rp);
		}
	}

	bmn->bmn_Drawed = TRUE;
	bmn->bmn_SaveX = x;
	bmn->bmn_SaveY = y;
	bmn->bmn_SaveWidth = width;
	bmn->bmn_SaveHeight = height;
//	bmn->bmn_SaveOffX = offx;
//	bmn->bmn_SaveOffY = offy;
}
//-------------------------------------
VOID UndrawBitMapNode(struct BitMapNode *bmn )
{
	struct RastPort *rp = &bmn->bmn_DnD->dnd_Screen->RastPort;

	if( !bmn->bmn_SaveBitMap ) return;

	if( bmn->bmn_SaveWidth > 0 && bmn->bmn_SaveHeight > 0 )
	{
		SafeBltBitMapRastPort( bmn->bmn_SaveBitMap,0,0,
									rp, bmn->bmn_SaveX, bmn->bmn_SaveY, bmn->bmn_SaveWidth, bmn->bmn_SaveHeight, 0xc0 );
	}
	bmn->bmn_SaveWidth = 0;
	bmn->bmn_SaveHeight = 0;
}
//-------------------------------------

//-------------------------------------
struct DragNDrop *CreateDragNDropA( struct TagItem *tlist )
{
	struct DragNDrop *dnd = (struct DragNDrop*)AllocMem( sizeof(struct DragNDrop), MEMF_CLEAR );
	if( dnd )
	{
		NewList( (struct List*)&dnd->dnd_List);

/*		if(dnd->dnd_LayerInfo = NewLayerInfo()))
		{
			dnd->dnd_Screen = NULL
			dnd->dnd_TempBitMap = NULL;


			struct RastPort *rp = &dnd->dnd_RastPort;
			InitRastPort(rp);

			rp->BitMap = 

			if(dnd->dnd_Layer = CreateBehindLayer(dnd->dnd_LayerInfo,
		
                        DeinitRastPort(rp);
			return dnd;
		}
		FreeMem( dnd, sizeof(struct DragNDrop ));*/
	}
	return dnd;
//	return NULL;
}
//-------------------------------------
VOID DeleteDragNDrop( struct DragNDrop *dnd )
{
	struct BitMapNode *node;

  FinishDragNDrop(dnd);

	while ((node = (struct BitMapNode *)RemTail((struct List*)&dnd->dnd_List)))
		DeleteBitMapNode(node);

	FreeMem( dnd, sizeof(struct DragNDrop ));
}
//-------------------------------------
VOID DrawDragNDrop(struct DragNDrop *dnd, LONG x, LONG y)
{
	static LONG lastx;
	static LONG lasty;
	static LONG first=TRUE;

	struct Screen *scr;
	struct RastPort *rp;
	struct BitMapNode *node;
	BOOL reverse;
	LONG diffx = x - lastx;
	LONG diffy = y - lasty;

	if(!dnd || !dnd->dnd_Screen) return;

	scr = dnd->dnd_Screen;
	rp = &scr->RastPort;

	reverse = FALSE;

	if(abs(diffy) < abs(diffx))//y==lasty)
	{
		if(diffx>0) reverse = TRUE;
		List_Sort_Mode_3(&dnd->dnd_List);
	}	else
	{
		if(diffy>0) reverse = TRUE;
		List_Sort_Mode_1(&dnd->dnd_List);
	}


/*	if(first) reverse = FALSE;
	else
	{
		if( x<lastx) reverse = FALSE;
		else
		{
			if(x==lastx && y < lasty) reverse=FALSE;
			else reverse = TRUE;
		}

		if(x>lastx && y < lasty)
		{
			List_Sort_Mode_2(&dnd->dnd_List);
			reverse=FALSE;
		}	else
		{
			if(x<lastx && y > lasty)
			{
				List_Sort_Mode_2(&dnd->dnd_List);
				reverse=TRUE;
			}
		}


//		cout << x << "  " << lastx << "  " << y << "  " << lasty << " " << reverse << endl;
	}*/

	node = List_First(&dnd->dnd_List);
	while(node)
	{
		node->bmn_Drawed = FALSE;
		node = Node_Next(node);
	}

	if(!reverse)
	{
		node = List_First(&dnd->dnd_List);
		while(node)
		{
			DrawBitMapNode( node, x + node->bmn_Left, y + node->bmn_Top);
			node = Node_Next(node);
		}
	}	else
	{
		node = (struct BitMapNode *)List_Last(&dnd->dnd_List);
		while(node)
		{
			DrawBitMapNode(node, x + node->bmn_Left, y + node->bmn_Top);
			node = (struct BitMapNode *)Node_Prev(node);
		}
	}
	first = FALSE;
	lastx = x;
	lasty = y;
}
//-------------------------------------
VOID UndrawDragNDrop(struct DragNDrop *dnd)
{
	struct BitMapNode *node;
	node = (struct BitMapNode *)List_Last(&dnd->dnd_List);
	while(node)
	{
		UndrawBitMapNode(node);
		node =  (struct BitMapNode *)Node_Prev(node);
	}
}
//-------------------------------------
BOOL PrepareDragNDrop(struct DragNDrop *dnd,struct Screen *scr)
{
	struct BitMapNode *bmn;
	struct RastPort *rp;
	LONG depth;
	LONG maxwidth=0,maxheight=0;
	BOOL ok=TRUE;

	if(!dnd || !scr) return FALSE;
	dnd->dnd_Screen = scr;

	rp = &scr->RastPort;
	depth = GetBitMapAttr( rp->BitMap, BMA_DEPTH );

	bmn = List_First(&dnd->dnd_List);
	while(bmn)
	{
		bmn->bmn_SaveWidth = bmn->bmn_SaveHeight = 0;
		if( bmn->bmn_Width > maxwidth ) maxwidth=bmn->bmn_Width;
		if( bmn->bmn_Height > maxheight ) maxheight=bmn->bmn_Height;

		if( !(bmn->bmn_SaveBitMap = AllocBitMap( bmn->bmn_Width, bmn->bmn_Height, depth, BMF_MINPLANES, rp->BitMap )))
		{
			ok = FALSE;
			break;
		}
		bmn = Node_Next(bmn);
	}

	if(ok && maxwidth && maxheight)
	{
		dnd->dnd_TempBitMap = /*NULL;// */ AllocBitMap(maxwidth,maxheight,depth,BMF_MINPLANES,rp->BitMap);
		return TRUE;
	}

	bmn = List_First(&dnd->dnd_List);
	while(bmn)
	{
		if(bmn->bmn_SaveBitMap)
		{
			FreeBitMap(bmn->bmn_SaveBitMap);
			bmn->bmn_SaveBitMap = NULL;
		}
		bmn = Node_Next(bmn);
	}

	return FALSE;
}
//-------------------------------------
VOID FinishDragNDrop(struct DragNDrop *dnd)
{
	struct BitMapNode *bmn;
	if(dnd->dnd_TempBitMap) FreeBitMap(dnd->dnd_TempBitMap);

	bmn = List_First(&dnd->dnd_List);
	while(bmn)
	{
		if(bmn->bmn_SaveBitMap)
		{
			FreeBitMap(bmn->bmn_SaveBitMap);
			bmn->bmn_SaveBitMap = NULL;
		}
		bmn = Node_Next(bmn);
	}

}
//-------------------------------------

/**********************************************************************
 Varargs function of CreateBitMapNode(). Note that we need a dummy
 because we need at least one parameter
**********************************************************************/
struct BitMapNode *VARARGS68K CreateBitMapNode(void *dummy, ...)
{
#ifndef __amigaos4__
    return CreateBitMapNodeA( (struct TagItem*)(((ULONG*)&dummy)+1));
#else
    va_list argptr;
    struct TagItem *tagList;
    struct BitMapNode *res;

    va_startlinear(argptr, dummy);
    tagList = va_getlinearva(argptr,struct TagItem *);
    res = CreateBitMapNodeA(tagList);
    va_end(argptr);
    return res;
#endif
}

/******************************************************************************/
















#if 0

struct Library *TimerBase;
struct TimerStruct
{
	struct MsgPort *msgport;
	struct timerequest *iorequest;
	struct Library *timerbase;
	ULONG sent;
};

//-------------------------------------
ASM VOID TIMER_DeleteTimer( register __a0 APTR t )
{
	if( t )
	{
		struct TimerStruct *timer = (struct TimerStruct*)t;
		if( timer )
		{
			if( timer->timerbase )
			{
				if( timer->sent )
				{
//					printf("Test1\n");
					AbortIO((struct IORequest*)timer->iorequest);
//					printf("Test2\n");
					WaitIO((struct IORequest*)timer->iorequest);
				}

				CloseDevice((struct IORequest*)timer->iorequest);
			}
			if( timer->iorequest ) DeleteIORequest(timer->iorequest);
			if( timer->msgport ) DeleteMsgPort(timer->msgport);
			FreeVec(timer);
		}
	}
}
//-------------------------------------
ASM APTR TIMER_CreateTimer()
{
	struct TimerStruct *timer = (struct TimerStruct *)AllocVec( sizeof(struct TimerStruct),0x10000);
	if( timer )
	{
		if(( timer->msgport = CreateMsgPort()))
		{
			if(( timer->iorequest = (struct timerequest*)CreateIORequest( timer->msgport, sizeof(struct timerequest))))
			{
				if( !OpenDevice( TIMERNAME, UNIT_VBLANK, (struct IORequest*)timer->iorequest, NULL ))
				{
#ifdef __MAXON__
					/*TimerBase = */timer->timerbase = (struct Library*)timer->iorequest->tr_node.io_Device;
#else
					timer->timerbase = (struct Library*)timer->iorequest->tr_node.io_Device;
#endif
					return timer;
				}
			}
		}
		TIMER_DeleteTimer(timer);
	}
	return NULL;
}
//-------------------------------------
ASM struct MsgPort *TIMER_GetMsgPort( register __a0 APTR t )
{
	if( !t ) return NULL;
	return ((struct TimerStruct *)t)->msgport;
}
//-------------------------------------
ASM ULONG TIMER_GetSigMask( register __a0 APTR t )
{
	if( !t ) return NULL;
	return (1UL << (((struct TimerStruct *)t)->msgport->mp_SigBit));
}
//-------------------------------------
ASM APTR TIMER_StartTimer( register __a0 APTR t, register __d0 ULONG secs, register __d1 ULONG mics )
{
	struct TimerStruct *timer;
	struct timerequest *req;

	if( !t ) return NULL;

	timer = (struct TimerStruct*)t;
	if( timer->sent ) return NULL;

	req = timer->iorequest;
	req->tr_node.io_Command = TR_ADDREQUEST;
	req->tr_time.tv_secs = secs;
	req->tr_time.tv_micro = mics;
	timer->sent = TRUE;
	SendIO((struct IORequest*)req );
	return (APTR)1L;
}
//-------------------------------------
ASM VOID TIMER_StopTimer( register __a0 APTR t )
{
	struct TimerStruct *timer;
	if( !t ) return;

	timer = (struct TimerStruct*)t;
	if( timer->sent )
	{
		AbortIO((struct IORequest*)timer->iorequest);
		WaitIO((struct IORequest*)timer->iorequest);
		timer->sent = 0;
	}
}
//-------------------------------------

//-------------------------------------
struct BitMap *CreateBitmapFromIcon(struct Screen *scr, struct DiskObject *dobj, LONG *width, LONG *height)
{
	struct Rectangle rect;
	static struct TagItem rect_tags[] =
	{
		ICONDRAWA_Borderless, TRUE,
		TAG_DONE,0
	};

	static struct TagItem draw_tags[] =
	{
		ICONDRAWA_Borderless,TRUE,
		ICONDRAWA_EraseBackground,TRUE,
		TAG_DONE,0
	};

	if(!dobj) return NULL;

	if(GetIconRectangleA(NULL,dobj,NULL,&rect,rect_tags))
	{
		BOOL standard;
		struct BitMap *bmap;
		if(GetBitMapAttr(scr->RastPort.BitMap,BMA_FLAGS) & BMF_STANDARD) standard = TRUE;
		else standard = FALSE;

		*width = rect.MaxX - rect.MinX + 1;
		*height = rect.MaxY - rect.MinY + 1;

//		cout << rect.MinY << " " << rect.MaxY << endl;

		bmap = AllocBitMap(*width,*height,8,/*NULL,NULL);// */ BMF_MINPLANES, standard?NULL:scr->RastPort.BitMap);
		if(bmap)
		{
			struct RastPort rp;
			InitRastPort(&rp);
			rp.BitMap = bmap;
			SetRast(&rp,1);
			DrawIconStateA(&rp,dobj,NULL,0,0,IDS_SELECTED,draw_tags);
			DeinitRastPort(&rp);
			
                        return bmap;
		}
	}

	return NULL;
}
//-------------------------------------

struct Window *wnd;
struct DragNDrop *drag;

//-------------------------------------
VOID loop()
{
	BOOL ready = FALSE;
	static LONG lmx,lmy;

	WaitPort( wnd->UserPort );
	while( ready == FALSE )
	{
		struct IntuiMessage *imsg;
		while((imsg = (struct IntuiMessage*)GetMsg( wnd->UserPort )))
		{
			ULONG cl = imsg->Class;
			UWORD code = imsg->Code;
//			LONG mx = imsg->MouseX;
//			LONG my = imsg->MouseY;
	
			ReplyMsg((struct Message *)imsg);
	
			switch( cl )
			{
				case	IDCMP_CLOSEWINDOW:
							ready = TRUE;
							break;

				case	IDCMP_VANILLAKEY:
							{
								DrawDragNDrop(drag,wnd->WScreen->MouseX,wnd->WScreen->MouseY);
								lmx = wnd->WScreen->MouseX;
								lmy = wnd->WScreen->MouseY;
							}
							break;

				case	IDCMP_MOUSEBUTTONS:
							if(code == SELECTDOWN)
							{
								DrawDragNDrop(drag,wnd->WScreen->MouseX,wnd->WScreen->MouseY);
								lmx = wnd->WScreen->MouseX;
								lmy = wnd->WScreen->MouseY;
							}
							break;
				
				case	IDCMP_MOUSEMOVE:
//							cout << wnd->WScreen->MouseX - lmx << "  " << wnd->WScreen->MouseY - lmy << endl;
//							WaitBOVP(&wnd->WScreen->ViewPort);
							DrawDragNDrop(drag,wnd->WScreen->MouseX,wnd->WScreen->MouseY);
							break;
			}
		}
	}
}
//-------------------------------------

UBYTE fullmask[8192];

void main()
{
  int i;
  for (i=0;i<8192;i++) fullmask[i] = 0xff;

	wnd = OpenWindowTags( NULL,
													WA_InnerWidth, 400,
													WA_InnerHeight, 200,
													WA_IDCMP, IDCMP_CLOSEWINDOW|IDCMP_MOUSEMOVE|IDCMP_INTUITICKS|IDCMP_MOUSEBUTTONS|IDCMP_VANILLAKEY,
													WA_DragBar, TRUE,
													WA_DepthGadget, TRUE,
													WA_CloseGadget, TRUE,
													WA_ReportMouse, TRUE,
													WA_Activate, TRUE,
													WA_GimmeZeroZero, TRUE,
													WA_MouseQueue, 2,
													TAG_DONE );
	if( wnd )
	{
		BOOL ready = FALSE;
		struct DiskObject *obj1 = GetIconTags("SYS:Prefs",
					ICONGETA_GenerateImageMasks,TRUE,
					TAG_DONE);
		struct DiskObject *obj2 = GetIconTags("SYS:Picasso96",
					ICONGETA_GenerateImageMasks,TRUE,
					TAG_DONE);
		struct DiskObject *obj3 = GetIconTags("SYS:Tools",
					ICONGETA_GenerateImageMasks,TRUE,
					TAG_DONE);
		LONG width,height;
		struct BitMap *bmap1 = CreateBitmapFromIcon(wnd->WScreen, obj1,&width,&height);
		struct BitMap *bmap2 = CreateBitmapFromIcon(wnd->WScreen, obj2,&width,&height);
		struct BitMap *bmap3 = CreateBitmapFromIcon(wnd->WScreen, obj3,&width,&height);
		if(bmap1&&bmap2&&bmap3)
		{
			APTR mask1,mask2,mask3;
			IconControl(obj1,ICONCTRLA_GetImageMask1,&mask1,TAG_DONE);
			IconControl(obj2,ICONCTRLA_GetImageMask1,&mask2,TAG_DONE);
			IconControl(obj3,ICONCTRLA_GetImageMask1,&mask3,TAG_DONE);
			if((drag = CreateDragNDropA(NULL)))
			{
				struct BitMapNode *bmn1 = CreateBitMapNode(
							GUI_BitMap, bmap1,
							GUI_Mask, mask1,
							GUI_Width, width,
							GUI_Height, height,
							GUI_TopOffset, -25,
							GUI_LeftOffset, -35,
							TAG_DONE);

				struct BitMapNode *bmn2 = CreateBitMapNode(
							GUI_BitMap, bmap2,
							GUI_Mask, mask2,
							GUI_Width, width,
							GUI_Height, height,
							GUI_LeftOffset, 0,
							TAG_DONE);

				struct BitMapNode *bmn3 = CreateBitMapNode(
							GUI_BitMap, bmap3,
							GUI_Mask, mask3,
							GUI_Width, width,
							GUI_Height, height,
							GUI_LeftOffset, 99,
							GUI_TopOffset, -10,
							TAG_DONE);

				struct BitMapNode *bmn4 = CreateBitMapNode(
							GUI_BitMap, bmap1,
							GUI_Mask, mask1,
							GUI_Width, width,
							GUI_Height, height,
							GUI_TopOffset, 60,
							TAG_DONE);

				struct BitMapNode *bmn5 = CreateBitMapNode(
							GUI_BitMap, bmap2,
							GUI_Mask, mask2,
							GUI_Width, width,
							GUI_Height, height,
							GUI_LeftOffset, 50,
							GUI_TopOffset, 60,
							TAG_DONE);

				struct BitMapNode *bmn6 = CreateBitMapNode(
							GUI_BitMap, bmap3,
							GUI_Mask, mask3,
							GUI_Width, width,
							GUI_Height, height,
							GUI_LeftOffset, 100,
							GUI_TopOffset, 70,
							TAG_DONE);
	
				if(bmn1 && bmn2 && bmn3 && bmn4 && bmn5 && bmn6)
				{
					AttachBitMapNode(drag,bmn1);
					AttachBitMapNode(drag,bmn2);
					AttachBitMapNode(drag,bmn3);
					AttachBitMapNode(drag,bmn4);
					AttachBitMapNode(drag,bmn5);
					AttachBitMapNode(drag,bmn6);
					PrepareDragNDrop(drag,wnd->WScreen);
					loop();
					FinishDragNDrop(drag);
					DetachBitMapNode(bmn6);
					DetachBitMapNode(bmn5);
					DetachBitMapNode(bmn4);
					DetachBitMapNode(bmn3);
					DetachBitMapNode(bmn2);
					DetachBitMapNode(bmn1);
				}
				if(bmn6) DeleteBitMapNode(bmn6);
				if(bmn5) DeleteBitMapNode(bmn5);
				if(bmn4) DeleteBitMapNode(bmn4);
				if(bmn3) DeleteBitMapNode(bmn3);
				if(bmn2) DeleteBitMapNode(bmn2);
				if(bmn1) DeleteBitMapNode(bmn1);
				DeleteDragNDrop(drag);
			}
		}

		if(bmap3) FreeBitMap(bmap3);
		if(bmap2) FreeBitMap(bmap2);
		if(bmap1) FreeBitMap(bmap1);

		if(obj3) FreeDiskObject(obj3);
		if(obj2) FreeDiskObject(obj2);
		if(obj1) FreeDiskObject(obj1);
		CloseWindow( wnd );
	}
}

//-------------------------------------

#endif
