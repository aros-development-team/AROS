#ifndef DEFINES_GRAPHICS_H
#define DEFINES_GRAPHICS_H

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AddFont(textFont) \
    AROS_LC1(void, AddFont, \
    AROS_LCA(struct TextFont *, textFont, A1), \
    struct GfxBase *, GfxBase, 80, Graphics)

#define AllocBitMap(sizex, sizey, depth, flags, friend_bitmap) \
    AROS_LC5(struct BitMap *, AllocBitMap, \
    AROS_LCA(ULONG          , sizex, D0), \
    AROS_LCA(ULONG          , sizey, D1), \
    AROS_LCA(ULONG          , depth, D2), \
    AROS_LCA(ULONG          , flags, D3), \
    AROS_LCA(struct BitMap *, friend_bitmap, A0), \
    struct GfxBase *, GfxBase, 153, Graphics)

#define AllocRaster(width, height) \
    AROS_LC2(PLANEPTR, AllocRaster, \
    AROS_LCA(ULONG          , width, D0), \
    AROS_LCA(ULONG          , height, D1), \
    struct GfxBase *, GfxBase, 82, Graphics)

#define AndRectRegion(region, rectangle) \
    AROS_LC2(void, AndRectRegion, \
    AROS_LCA(struct Region    *, region, A0), \
    AROS_LCA(struct Rectangle *, rectangle, A1), \
    struct GfxBase *, GfxBase, 84, Graphics)

#define AreaDraw(rp, x, y) \
    AROS_LC3(LONG, AreaDraw, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 43, Graphics)

#define AreaEllipse(rp, xCenter, yCenter, a, b) \
    AROS_LC5(LONG, AreaEllipse, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , xCenter, D0), \
    AROS_LCA(LONG             , yCenter, D1), \
    AROS_LCA(LONG             , a, D2), \
    AROS_LCA(LONG             , b, D3), \
    struct GfxBase *, GfxBase, 31, Graphics)

#define AreaEnd(rp) \
    AROS_LC1(LONG, AreaEnd, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 44, Graphics)

#define AreaMove(rp, x, y) \
    AROS_LC3(LONG, AreaMove, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 42, Graphics)

#define AskFont(rp, textAttr) \
    AROS_LC2(void, AskFont, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(struct TextAttr *, textAttr, A0), \
    struct GfxBase *, GfxBase, 79, Graphics)

#define BltBitMap(srcBitMap, xSrc, ySrc, destBitMap, xDest, yDest, xSize, ySize, minterm, mask, tempA) \
    AROS_LC11(LONG, BltBitMap, \
    AROS_LCA(struct BitMap *, srcBitMap, A0), \
    AROS_LCA(LONG           , xSrc, D0), \
    AROS_LCA(LONG           , ySrc, D1), \
    AROS_LCA(struct BitMap *, destBitMap, A1), \
    AROS_LCA(LONG           , xDest, D2), \
    AROS_LCA(LONG           , yDest, D3), \
    AROS_LCA(LONG           , xSize, D4), \
    AROS_LCA(LONG           , ySize, D5), \
    AROS_LCA(ULONG          , minterm, D6), \
    AROS_LCA(ULONG          , mask, D7), \
    AROS_LCA(PLANEPTR       , tempA, A2), \
    struct GfxBase *, GfxBase, 5, Graphics)

#define ClearRectRegion(region, rectangle) \
    AROS_LC2(BOOL, ClearRectRegion, \
    AROS_LCA(struct Region    *, region   , A0), \
    AROS_LCA(struct Rectangle *, rectangle, A1), \
    struct GfxBase *, GfxBase, 87, Graphics)

#define ClearRegion(region) \
    AROS_LC1(void, ClearRegion, \
    AROS_LCA(struct Region *, region, A0), \
    struct GfxBase *, GfxBase, 88, Graphics)

#define CloneRastPort(rp) \
    AROS_LC1(struct RastPort *, CloneRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 178, Graphics)

#define CloseFont(textFont) \
    AROS_LC1(void, CloseFont, \
    AROS_LCA(struct TextFont *, textFont, A1), \
    struct GfxBase *, GfxBase, 13, Graphics)

#define CreateRastPort() \
    AROS_LC0(struct RastPort *, CreateRastPort, \
    struct GfxBase *, GfxBase, 177, Graphics)

#define DeinitRastPort(rp) \
    AROS_LC1(void, DeinitRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 179, Graphics)

#define DisposeRegion(region) \
    AROS_LC1(void, DisposeRegion, \
    AROS_LCA(struct Region *, region, A0), \
    struct GfxBase *, GfxBase, 89, Graphics)

#define Draw(rp, x, y) \
    AROS_LC3(void, Draw, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 41, Graphics)

#define DrawEllipse(rp, xCenter, yCenter, a, b) \
    AROS_LC5(void, DrawEllipse, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , xCenter, D0), \
    AROS_LCA(LONG             , yCenter, D1), \
    AROS_LCA(LONG             , a, D2), \
    AROS_LCA(LONG             , b, D3), \
    struct GfxBase *, GfxBase, 30, Graphics)

#define EraseRect(rp, xMin, yMin, xMax, yMax) \
    AROS_LC5(void, EraseRect, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , xMin, D0), \
    AROS_LCA(LONG             , yMin, D1), \
    AROS_LCA(LONG             , xMax, D2), \
    AROS_LCA(LONG             , yMax, D3), \
    struct GfxBase *, GfxBase, 135, Graphics)

#define FreeBitMap(bm) \
    AROS_LC1(void, FreeBitMap, \
    AROS_LCA(struct BitMap *, bm, A0), \
    struct GfxBase *, GfxBase, 154, Graphics)

#define FreeRaster(p, width, height) \
    AROS_LC3(void, FreeRaster, \
    AROS_LCA(PLANEPTR, p,      A0), \
    AROS_LCA(ULONG,    width,  D0), \
    AROS_LCA(ULONG,    height, D1), \
    struct GfxBase *, GfxBase, 83, Graphics)

#define FreeRastPort(rp) \
    AROS_LC1(void, FreeRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 180, Graphics)

#define GetAPen(rp) \
    AROS_LC1(ULONG, GetAPen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 143, Graphics)

#define GetBPen(rp) \
    AROS_LC1(ULONG, GetBPen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 144, Graphics)

#define GetDrMd(rp) \
    AROS_LC1(ULONG, GetDrMd, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 145, Graphics)

#define GetOutlinePen(rp) \
    AROS_LC1(ULONG, GetOutlinePen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 146, Graphics)

#define InitArea(areaInfo, vectorBuffer, maxVectors) \
    AROS_LC3(void, InitArea, \
    AROS_LCA(struct AreaInfo *, areaInfo, A0), \
    AROS_LCA(APTR             , vectorBuffer, A1), \
    AROS_LCA(LONG             , maxVectors, D0), \
    struct GfxBase *, GfxBase, 47, Graphics)

#define InitBitMap(bitMap, depth, width, height) \
    AROS_LC4(void, InitBitMap, \
    AROS_LCA(struct BitMap *, bitMap, A0), \
    AROS_LCA(LONG           , depth, D0), \
    AROS_LCA(LONG           , width, D1), \
    AROS_LCA(LONG           , height, D2), \
    struct GfxBase *, GfxBase, 65, Graphics)

#define InitRastPort(rp) \
    AROS_LC1(BOOL, InitRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 33, Graphics)

#define InitTmpRas(tmpRas, PLANEPTR, buffer, size) \
    AROS_LC4(struct TmpRas *, InitTmpRas, \
    AROS_LCA(struct TmpRas *, tmpRas, A0), \
    AROS_LCA(               , PLANEPTR, A1), \
    AROS_LCA(               , buffer, D0), \
    AROS_LCA(LONG           , size, ), \
    struct GfxBase *, GfxBase, 78, Graphics)

#define LoadRGB32(vp, table) \
    AROS_LC2(void, LoadRGB32, \
    AROS_LCA(struct ViewPort *, vp, A0), \
    AROS_LCA(ULONG           *, table, A1), \
    struct GfxBase *, GfxBase, 147, Graphics)

#define LoadRGB4(vp, colors, count) \
    AROS_LC3(void, LoadRGB4, \
    AROS_LCA(struct ViewPort *, vp, A0), \
    AROS_LCA(UWORD           *, colors, A1), \
    AROS_LCA(LONG             , count, D0), \
    struct GfxBase *, GfxBase, 32, Graphics)

#define Move(rp, x, y) \
    AROS_LC3(void, Move, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 40, Graphics)

#define NewRegion() \
    AROS_LC0(struct Region *, NewRegion, \
    struct GfxBase *, GfxBase, 86, Graphics)

#define OpenFont(textAttr) \
    AROS_LC1(struct TextFont *, OpenFont, \
    AROS_LCA(struct TextAttr *, textAttr, A0), \
    struct GfxBase *, GfxBase, 12, Graphics)

#define OrRectRegion(region, rectangle) \
    AROS_LC2(BOOL, OrRectRegion, \
    AROS_LCA(struct Region    *, region,    A0), \
    AROS_LCA(struct Rectangle *, rectangle, A1), \
    struct GfxBase *, GfxBase, 85, Graphics)

#define PolyDraw(rp, count, polyTable) \
    AROS_LC3(void, PolyDraw, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , count, D0), \
    AROS_LCA(WORD            *, polyTable, A0), \
    struct GfxBase *, GfxBase, 56, Graphics)

#define ReadPixel(rp, x, y) \
    AROS_LC3(ULONG, ReadPixel, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 53, Graphics)

#define RectFill(rp, xMin, yMin, xMax, yMax) \
    AROS_LC5(void, RectFill, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , xMin, D0), \
    AROS_LCA(LONG             , yMin, D1), \
    AROS_LCA(LONG             , xMax, D2), \
    AROS_LCA(LONG             , yMax, D3), \
    struct GfxBase *, GfxBase, 51, Graphics)

#define RemFont(textFont) \
    AROS_LC1(void, RemFont, \
    AROS_LCA(struct TextFont *, textFont, A1), \
    struct GfxBase *, GfxBase, 81, Graphics)

#define ScrollRaster(rp, dx, dy, xMin, yMin, xMax, yMax) \
    AROS_LC7(void, ScrollRaster, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , dx, D0), \
    AROS_LCA(LONG             , dy, D1), \
    AROS_LCA(LONG             , xMin, D2), \
    AROS_LCA(LONG             , yMin, D3), \
    AROS_LCA(LONG             , xMax, D4), \
    AROS_LCA(LONG             , yMax, D5), \
    struct GfxBase *, GfxBase, 66, Graphics)

#define SetABPenDrMd(rp, apen, bpen, drawMode) \
    AROS_LC4(void, SetABPenDrMd, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , apen, D0), \
    AROS_LCA(ULONG            , bpen, D1), \
    AROS_LCA(ULONG            , drawMode, D2), \
    struct GfxBase *, GfxBase, 149, Graphics)

#define SetAPen(rp, pen) \
    AROS_LC2(void, SetAPen, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , pen, D0), \
    struct GfxBase *, GfxBase, 57, Graphics)

#define SetBPen(rp, pen) \
    AROS_LC2(void, SetBPen, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , pen, D0), \
    struct GfxBase *, GfxBase, 58, Graphics)

#define SetDrMd(rp, drawMode) \
    AROS_LC2(void, SetDrMd, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , drawMode, D0), \
    struct GfxBase *, GfxBase, 59, Graphics)

#define SetFont(rp, textFont) \
    AROS_LC2(void, SetFont, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(struct TextFont *, textFont, A0), \
    struct GfxBase *, GfxBase, 11, Graphics)

#define SetOutlinePen(rp, pen) \
    AROS_LC2(ULONG, SetOutlinePen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(ULONG,             pen, D0), \
    struct GfxBase *, GfxBase, 163, Graphics)

#define SetRast(rp, pen) \
    AROS_LC2(void, SetRast, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , pen, D0), \
    struct GfxBase *, GfxBase, 39, Graphics)

#define SetRGB32(vp, n, r, g, b) \
    AROS_LC5(void, SetRGB32, \
    AROS_LCA(struct ViewPort *, vp, A0), \
    AROS_LCA(ULONG            , n, D0), \
    AROS_LCA(ULONG            , r, D1), \
    AROS_LCA(ULONG            , g, D2), \
    AROS_LCA(ULONG            , b, D3), \
    struct GfxBase *, GfxBase, 142, Graphics)

#define SetRPAttrsA(rp, tags) \
    AROS_LC2(void, SetRPAttrsA, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct TagItem  *, tags, A1), \
    struct GfxBase *, GfxBase, 173, Graphics)

#define SetWriteMask(rp, mask) \
    AROS_LC2(ULONG, SetWriteMask, \
    AROS_LCA(struct RastPort *, rp,   A0), \
    AROS_LCA(ULONG            , mask, D0), \
    struct GfxBase *, GfxBase, 164, Graphics)

#define Text(rp, string, count) \
    AROS_LC3(void, Text, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(STRPTR           , string, A0), \
    AROS_LCA(ULONG            , count, D0), \
    struct GfxBase *, GfxBase, 10, Graphics)

#define TextFit(rp, string, strLen, textExtent, constrainingExtent, strDirection, constrainingBitWidth, constrainingBitHeight) \
    AROS_LC8(ULONG, TextFit, \
    AROS_LCA(struct RastPort   *, rp, A1), \
    AROS_LCA(STRPTR             , string, A0), \
    AROS_LCA(ULONG              , strLen, D0), \
    AROS_LCA(struct TextExtent *, textExtent, A2), \
    AROS_LCA(struct TextExtent *, constrainingExtent, A3), \
    AROS_LCA(LONG               , strDirection, D1), \
    AROS_LCA(ULONG              , constrainingBitWidth, D2), \
    AROS_LCA(ULONG              , constrainingBitHeight, D3), \
    struct GfxBase *, GfxBase, 116, Graphics)

#define TextLength(rp, string, count) \
    AROS_LC3(WORD, TextLength, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(STRPTR           , string, A0), \
    AROS_LCA(ULONG            , count, D0), \
    struct GfxBase *, GfxBase, 9, Graphics)

#define WaitTOF() \
    AROS_LC0(void, WaitTOF, \
    struct GfxBase *, GfxBase, 45, Graphics)

#define WritePixel(rp, x, y) \
    AROS_LC3(LONG, WritePixel, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 54, Graphics)

#define WritePixelArray8(rp, xstart, ystart, xstop, ystop, array, temprp) \
    AROS_LC7(LONG, WritePixelArray8, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(ULONG            , xstart, D0), \
    AROS_LCA(ULONG            , ystart, D1), \
    AROS_LCA(ULONG            , xstop, D2), \
    AROS_LCA(ULONG            , ystop, D3), \
    AROS_LCA(UBYTE           *, array, A2), \
    AROS_LCA(struct RastPort *, temprp, A1), \
    struct GfxBase *, GfxBase, 131, Graphics)

#define XorRectRegion(region, rectangle) \
    AROS_LC2(BOOL, XorRectRegion, \
    AROS_LCA(struct Region    *, region,    A0), \
    AROS_LCA(struct Rectangle *, rectangle, A1), \
    struct GfxBase *, GfxBase, 93, Graphics)

#endif /* DEFINES_GRAPHICS_H */
