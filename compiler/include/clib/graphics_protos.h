#ifndef CLIB_GRAPHICS_PROTOS_H
#define CLIB_GRAPHICS_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for graphics.library
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

struct View;
/*
    Prototypes
*/
AROS_LP1(void, AddFont,
    AROS_LPA(struct TextFont *, textFont, A1),
    struct GfxBase *, GfxBase, 80, Graphics)

AROS_LP5(struct BitMap *, AllocBitMap,
    AROS_LPA(ULONG          , sizex, D0),
    AROS_LPA(ULONG          , sizey, D1),
    AROS_LPA(ULONG          , depth, D2),
    AROS_LPA(ULONG          , flags, D3),
    AROS_LPA(struct BitMap *, friend_bitmap, A0),
    struct GfxBase *, GfxBase, 153, Graphics)

AROS_LP2(PLANEPTR, AllocRaster,
    AROS_LPA(ULONG          , width, D0),
    AROS_LPA(ULONG          , height, D1),
    struct GfxBase *, GfxBase, 82, Graphics)

AROS_LP2(void, AndRectRegion,
    AROS_LPA(struct Region *, region, A0),
    AROS_LPA(struct Rectangle *, rectangle, A1),
    struct GfxBase *, GfxBase, 84, Graphics)

AROS_LP3(LONG, AreaDraw,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 43, Graphics)

AROS_LP5(LONG, AreaEllipse,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , xCenter, D0),
    AROS_LPA(LONG             , yCenter, D1),
    AROS_LPA(LONG             , a, D2),
    AROS_LPA(LONG             , b, D3),
    struct GfxBase *, GfxBase, 31, Graphics)

AROS_LP1(LONG, AreaEnd,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 44, Graphics)

AROS_LP3(LONG, AreaMove,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 42, Graphics)

AROS_LP2(void, AskFont,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(struct TextAttr *, textAttr, A0),
    struct GfxBase *, GfxBase, 79, Graphics)

AROS_LP11(LONG, BltBitMap,
    AROS_LPA(struct BitMap *, srcBitMap, A0),
    AROS_LPA(LONG           , xSrc, D0),
    AROS_LPA(LONG           , ySrc, D1),
    AROS_LPA(struct BitMap *, destBitMap, A1),
    AROS_LPA(LONG           , xDest, D2),
    AROS_LPA(LONG           , yDest, D3),
    AROS_LPA(LONG           , xSize, D4),
    AROS_LPA(LONG           , ySize, D5),
    AROS_LPA(ULONG          , minterm, D6),
    AROS_LPA(ULONG          , mask, D7),
    AROS_LPA(PLANEPTR       , tempA, A2),
    struct GfxBase *, GfxBase, 5, Graphics)

AROS_LP2(BOOL, ClearRectRegion,
    AROS_LPA(struct Region *, region, A0),
    AROS_LPA(struct Rectangle *, rectangle, A1),
    struct GfxBase *, GfxBase, 87, Graphics)

AROS_LP1(struct RastPort *, CloneRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 178, Graphics)

AROS_LP1(void, CloseFont,
    AROS_LPA(struct TextFont *, textFont, A1),
    struct GfxBase *, GfxBase, 13, Graphics)

AROS_LP0(struct RastPort *, CreateRastPort,
    struct GfxBase *, GfxBase, 177, Graphics)

AROS_LP1(void, DeinitRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 179, Graphics)

AROS_LP1(void, DisposeRegion,
    AROS_LPA(struct Region *, region, A0),
    struct GfxBase *, GfxBase, 89, Graphics)

AROS_LP3(void, Draw,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 41, Graphics)

AROS_LP5(void, DrawEllipse,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , xCenter, D0),
    AROS_LPA(LONG             , yCenter, D1),
    AROS_LPA(LONG             , a, D2),
    AROS_LPA(LONG             , b, D3),
    struct GfxBase *, GfxBase, 30, Graphics)

AROS_LP5(void, EraseRect,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , xMin, D0),
    AROS_LPA(LONG             , yMin, D1),
    AROS_LPA(LONG             , xMax, D2),
    AROS_LPA(LONG             , yMax, D3),
    struct GfxBase *, GfxBase, 135, Graphics)

AROS_LP1(void, FreeBitMap,
    AROS_LPA(struct BitMap *, bm, A0),
    struct GfxBase *, GfxBase, 154, Graphics)

AROS_LP3(void, FreeRaster,
    AROS_LPA(PLANEPTR, p,      A0),
    AROS_LPA(ULONG,    width,  D0),
    AROS_LPA(ULONG,    height, D1),
    struct GfxBase *, GfxBase, 83, Graphics)

AROS_LP1(void, FreeRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 180, Graphics)

AROS_LP1(ULONG, GetAPen,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 143, Graphics)

AROS_LP1(ULONG, GetBPen,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 144, Graphics)

AROS_LP1(ULONG, GetDrMd,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 145, Graphics)

AROS_LP1(ULONG, GetOutlinePen,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 146, Graphics)

AROS_LP3(void, InitArea,
    AROS_LPA(struct AreaInfo *, areaInfo, A0),
    AROS_LPA(APTR             , vectorBuffer, A1),
    AROS_LPA(LONG             , maxVectors, D0),
    struct GfxBase *, GfxBase, 47, Graphics)

AROS_LP4(void, InitBitMap,
    AROS_LPA(struct BitMap *, bitMap, A0),
    AROS_LPA(LONG           , depth, D0),
    AROS_LPA(LONG           , width, D1),
    AROS_LPA(LONG           , height, D2),
    struct GfxBase *, GfxBase, 65, Graphics)

AROS_LP1(BOOL, InitRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 33, Graphics)

AROS_LP4(struct TmpRas *, InitTmpRas,
    AROS_LPA(struct TmpRas *, tmpRas, A0),
    AROS_LPA(               , PLANEPTR, A1),
    AROS_LPA(               , buffer, D0),
    AROS_LPA(LONG           , size, ),
    struct GfxBase *, GfxBase, 78, Graphics)

AROS_LP2(void, LoadRGB32,
    AROS_LPA(struct ViewPort *, vp, A0),
    AROS_LPA(ULONG           *, table, A1),
    struct GfxBase *, GfxBase, 147, Graphics)

AROS_LP3(void, LoadRGB4,
    AROS_LPA(struct ViewPort *, vp, A0),
    AROS_LPA(UWORD           *, colors, A1),
    AROS_LPA(LONG             , count, D0),
    struct GfxBase *, GfxBase, 32, Graphics)

AROS_LP3(void, Move,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 40, Graphics)

AROS_LP0(struct Region *, NewRegion,
    struct GfxBase *, GfxBase, 86, Graphics)

AROS_LP1(struct TextFont *, OpenFont,
    AROS_LPA(struct TextAttr *, textAttr, A0),
    struct GfxBase *, GfxBase, 12, Graphics)

AROS_LP2(BOOL, OrRectRegion,
    AROS_LPA(struct Region *, region, A0),
    AROS_LPA(struct Rectangle *, rectangle, A1),
    struct GfxBase *, GfxBase, 85, Graphics)

AROS_LP3(void, PolyDraw,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , count, D0),
    AROS_LPA(WORD            *, polyTable, A0),
    struct GfxBase *, GfxBase, 56, Graphics)

AROS_LP3(ULONG, ReadPixel,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 53, Graphics)

AROS_LP5(void, RectFill,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , xMin, D0),
    AROS_LPA(LONG             , yMin, D1),
    AROS_LPA(LONG             , xMax, D2),
    AROS_LPA(LONG             , yMax, D3),
    struct GfxBase *, GfxBase, 51, Graphics)

AROS_LP1(void, RemFont,
    AROS_LPA(struct TextFont *, textFont, A1),
    struct GfxBase *, GfxBase, 81, Graphics)

AROS_LP7(void, ScrollRaster,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , dx, D0),
    AROS_LPA(LONG             , dy, D1),
    AROS_LPA(LONG             , xMin, D2),
    AROS_LPA(LONG             , yMin, D3),
    AROS_LPA(LONG             , xMax, D4),
    AROS_LPA(LONG             , yMax, D5),
    struct GfxBase *, GfxBase, 66, Graphics)

AROS_LP4(void, SetABPenDrMd,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , apen, D0),
    AROS_LPA(ULONG            , bpen, D1),
    AROS_LPA(ULONG            , drawMode, D2),
    struct GfxBase *, GfxBase, 149, Graphics)

AROS_LP2(void, SetAPen,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , pen, D0),
    struct GfxBase *, GfxBase, 57, Graphics)

AROS_LP2(void, SetBPen,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , pen, D0),
    struct GfxBase *, GfxBase, 58, Graphics)

AROS_LP2(void, SetDrMd,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , drawMode, D0),
    struct GfxBase *, GfxBase, 59, Graphics)

AROS_LP2(void, SetFont,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(struct TextFont *, textFont, A0),
    struct GfxBase *, GfxBase, 11, Graphics)

AROS_LP2(ULONG, SetOutlinePen,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(ULONG,             pen, D0),
    struct GfxBase *, GfxBase, 163, Graphics)

AROS_LP2(void, SetRast,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , pen, D0),
    struct GfxBase *, GfxBase, 39, Graphics)

AROS_LP2(void, SetRPAttrsA,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(struct TagItem  *, tags, A1),
    struct GfxBase *, GfxBase, 173, Graphics)

AROS_LP2(ULONG, SetWriteMask,
    AROS_LPA(struct RastPort *, rp,   A0),
    AROS_LPA(ULONG            , mask, D0),
    struct GfxBase *, GfxBase, 164, Graphics)

AROS_LP3(void, Text,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(STRPTR           , string, A0),
    AROS_LPA(ULONG            , count, D0),
    struct GfxBase *, GfxBase, 10, Graphics)

AROS_LP8(ULONG, TextFit,
    AROS_LPA(struct RastPort   *, rp, A1),
    AROS_LPA(STRPTR             , string, A0),
    AROS_LPA(ULONG              , strLen, D0),
    AROS_LPA(struct TextExtent *, textExtent, A2),
    AROS_LPA(struct TextExtent *, constrainingExtent, A3),
    AROS_LPA(LONG               , strDirection, D1),
    AROS_LPA(ULONG              , constrainingBitWidth, D2),
    AROS_LPA(ULONG              , constrainingBitHeight, D3),
    struct GfxBase *, GfxBase, 116, Graphics)

AROS_LP3(WORD, TextLength,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(STRPTR           , string, A0),
    AROS_LPA(ULONG            , count, D0),
    struct GfxBase *, GfxBase, 9, Graphics)

AROS_LP0(void, WaitTOF,
    struct GfxBase *, GfxBase, 45, Graphics)

AROS_LP3(LONG, WritePixel,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 54, Graphics)


#endif /* CLIB_GRAPHICS_PROTOS_H */
