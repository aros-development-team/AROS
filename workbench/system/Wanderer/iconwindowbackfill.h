#ifndef _ICONWINDOWBACKFILL_H_
#define _ICONWINDOWBACKFILL_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <utility/hooks.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <datatypes/pictureclass.h>

#define MOD(x,y) ((x)<0 ? (y)-((-(x))%(y)) : (x)%(y))

#define RECTSIZEX(r) (((r)->MaxX - (r)->MinX)+1)
#define RECTSIZEY(r) (((r)->MaxY - (r)->MinY)+1)

#define blit_MODE_Blit                1
#define blit_MODE_Clip                2

/* TODO: Make Floating/Fixed tiles preference settable */
#define  TILE_MODE_FLOAT            0
#define  TILE_MODE_FIX              1

struct BackFillOptions
{
    IPTR                      bfo_MaxCopyWidth;
    IPTR                      bfo_MaxCopyHeight;
    IPTR                      bfo_OffsetX;
    IPTR                      bfo_OffsetY;
    IPTR                      bfo_TileMode;
};

struct BackFillSourceImageBuffer
{
    struct Node               bfsib_Node;
    int                        bfsib_OpenerCount;
    struct BitMap             *bfsib_BitMap;
    struct RastPort           *bfsib_BitMapRastPort;
    ULONG                     bfsib_BitMapWidth;
    ULONG                     bfsib_BitMapHeight;
};

struct BackFillSourceImageRecord
{
    /* Record and data for currently in use images */
    struct Node               bfsir_Node;
    int                       bfsir_OpenerCount;
    char                      *bfsir_SourceImage;                        /* Full name of image */
    
    IPTR                      bfsir_BackGroundRenderMode;
    
    Object                    *bfsir_DTPictureObject;
    struct RastPort           *bfsir_DTRastPort;
    struct BitMap             *bfsir_DTBitMap;
    struct BitMapHeader       *bfsir_DTBitMapHeader;

    struct List                  bfsir_Buffers;
};

struct BackFillInfo
{
    struct Screen                         *bfi_Screen;
    struct RastPort                       *bfi_RastPort;

    ULONG                                  bfi_CopyWidth;
    ULONG                                  bfi_CopyHeight;

    struct BackFillSourceImageRecord    *bfi_Source;
    struct BackFillSourceImageBuffer    *bfi_Buffer;
    struct BackFillOptions                bfi_Options;
};

#endif /* _ICONWINDOWBACKFILL_H_ */
