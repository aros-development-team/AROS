/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _DRAGNDROP_H
#define _DRAGNDROP_H

#ifndef _MUIMASTER_SUPPORT_H
#include "support.h"
#endif

/* Tags for GUI_CreateBitMapNodeA() */
#define GUI_BitMap				(TAG_USER+1)	/* struct BitMap * */
#define GUI_Mask					(TAG_USER+2)	/* APTR */
#define GUI_LeftOffset		(TAG_USER+3)	/* LONG */
#define GUI_TopOffset		(TAG_USER+4)	/* LONG */
#define GUI_Width				(TAG_USER+5)	/* LONG */
#define GUI_Height				(TAG_USER+6)	/* LONG */

struct DragNDrop *CreateDragNDropA( struct TagItem *tlist );
VOID DeleteDragNDrop( struct DragNDrop *dnd );

BOOL PrepareDragNDrop(struct DragNDrop *dnd,struct Screen *scr);
VOID FinishDragNDrop(struct DragNDrop *dnd);

VOID DrawDragNDrop(struct DragNDrop *dnd, LONG x, LONG y);
VOID UndrawDragNDrop(struct DragNDrop *dnd);

struct BitMapNode *CreateBitMapNodeA( struct TagItem *tagList );
struct BitMapNode *VARARGS68K CreateBitMapNode(void *dummy, ...);
VOID DeleteBitMapNode(struct BitMapNode *bmn );

VOID AttachBitMapNode( struct DragNDrop *dnd, struct BitMapNode *bmn );
VOID DetachBitMapNode( struct BitMapNode *bmn );

#endif
