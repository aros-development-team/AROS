#ifndef CLIB_GFXHIDD_PROTOS_H
#define CLIB_GFXHIDD_PROTOS_H

/*
    Copyright (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for GfxHidd
    Lang: english
*/

#include <exec/types.h>
#include <utility/tagitem.h>

/* Prototypes */
VOID HIDDF_Graphics_TestSpeed(ULONG val1, ULONG val2, ULONG val3);

APTR HIDD_Graphics_CreateBitMap(ULONG width, ULONG height, ULONG depth,
                                ULONG flags, APTR friendBitMap,
                                struct TagItem *tagList);
APTR HIDD_Graphics_CreateBitMapTags(ULONG width, ULONG height, ULONG depth,
                                ULONG flags, APTR friendBitMap,
                                unsigned long tag1Type, ... );
VOID HIDD_Graphics_DeleteBitMap(APTR bitMap, struct TagItem *tagList);
BOOL HIDD_Graphics_ShowBitMap(APTR bitMap, BOOL wait, struct TagItem *tagList);
VOID HIDD_Graphics_MoveBitMap(APTR bitMap, WORD horizontal, WORD vertical, struct TagItem *tagList);
ULONG HIDD_Graphics_DepthArrangeBitMap(APTR bitmap, ULONG mode, APTR otherBitMap, struct TagItem *tagList);
APTR HIDD_Graphics_CreateGC(APTR bitMap, struct TagItem *tagList);
VOID HIDD_Graphics_DeleteGC(APTR gc, struct TagItem *tagList);
BOOL HIDD_Graphics_WritePixelDirect(APTR gc, WORD x, WORD y, ULONG val);
BOOL HIDD_Graphics_WritePixel(APTR gc, WORD x, WORD y);
ULONG HIDD_Graphics_ReadPixel(APTR gc, WORD x, WORD y);
ULONG HIDD_Graphics_ReadPixel_Q(APTR gc, WORD x, WORD y);
VOID  HIDD_Graphics_WritePixelDirect_Q(APTR gc, WORD x, WORD y, ULONG val);
ULONG HIDD_Graphics_ReadPixelDirect(APTR gc, WORD x, WORD y);
ULONG HIDD_Graphics_ReadPixelDirect_Q(APTR gc, WORD x, WORD y);
VOID HIDD_Graphics_DrawLine_Q(APTR gc, WORD x1, WORD y1, WORD x2, WORD y2);
BOOL HIDD_Graphics_DrawLine(APTR gc, WORD x1, WORD y1, WORD x2, WORD y2);
VOID HIDD_Graphics_WritePixel_Q(APTR gc, WORD x, WORD y);

#endif /* CLIB_GFXHIDD_PROTOS_H */
