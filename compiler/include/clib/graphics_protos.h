#ifndef CLIB_GRAPHICS_PROTOS_H
#define CLIB_GRAPHICS_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP1(struct RastPort *, CloneRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 178, Graphics)
#define CloneRastPort(rp) \
    AROS_LC1(struct RastPort *, CloneRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 178, Graphics)

AROS_LP1(void, CloseFont,
    AROS_LPA(struct TextFont *, textFont, A1),
    struct GfxBase *, GfxBase, 13, Graphics)
#define CloseFont(textFont) \
    AROS_LC1(void, CloseFont, \
    AROS_LCA(struct TextFont *, textFont, A1), \
    struct GfxBase *, GfxBase, 13, Graphics)

AROS_LP0(struct RastPort *, CreateRastPort,
    struct GfxBase *, GfxBase, 177, Graphics)
#define CreateRastPort() \
    AROS_LC0(struct RastPort *, CreateRastPort, \
    struct GfxBase *, GfxBase, 177, Graphics)

AROS_LP1(void, DeinitRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 179, Graphics)
#define DeinitRastPort(rp) \
    AROS_LC1(void, DeinitRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 179, Graphics)

AROS_LP3(void, Draw,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 41, Graphics)
#define Draw(rp, x, y) \
    AROS_LC3(void, Draw, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 41, Graphics)

AROS_LP5(void, DrawEllipse,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , xCenter, D0),
    AROS_LPA(LONG             , yCenter, D1),
    AROS_LPA(LONG             , a, D2),
    AROS_LPA(LONG             , b, D3),
    struct GfxBase *, GfxBase, 30, Graphics)
#define DrawEllipse(rp, xCenter, yCenter, a, b) \
    AROS_LC5(void, DrawEllipse, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , xCenter, D0), \
    AROS_LCA(LONG             , yCenter, D1), \
    AROS_LCA(LONG             , a, D2), \
    AROS_LCA(LONG             , b, D3), \
    struct GfxBase *, GfxBase, 30, Graphics)

AROS_LP5(void, EraseRect,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , xMin, D0),
    AROS_LPA(LONG             , yMin, D1),
    AROS_LPA(LONG             , xMax, D2),
    AROS_LPA(LONG             , yMax, D3),
    struct GfxBase *, GfxBase, 135, Graphics)
#define EraseRect(rp, xMin, yMin, xMax, yMax) \
    AROS_LC5(void, EraseRect, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , xMin, D0), \
    AROS_LCA(LONG             , yMin, D1), \
    AROS_LCA(LONG             , xMax, D2), \
    AROS_LCA(LONG             , yMax, D3), \
    struct GfxBase *, GfxBase, 135, Graphics)

AROS_LP1(void, FreeRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 180, Graphics)
#define FreeRastPort(rp) \
    AROS_LC1(void, FreeRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 180, Graphics)

AROS_LP1(ULONG, GetAPen,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 143, Graphics)
#define GetAPen(rp) \
    AROS_LC1(ULONG, GetAPen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 143, Graphics)

AROS_LP1(ULONG, GetBPen,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 144, Graphics)
#define GetBPen(rp) \
    AROS_LC1(ULONG, GetBPen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 144, Graphics)

AROS_LP1(ULONG, GetDrMd,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 145, Graphics)
#define GetDrMd(rp) \
    AROS_LC1(ULONG, GetDrMd, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 145, Graphics)

AROS_LP1(ULONG, GetOutlinePen,
    AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 146, Graphics)
#define GetOutlinePen(rp) \
    AROS_LC1(ULONG, GetOutlinePen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 146, Graphics)

AROS_LP1(BOOL, InitRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 33, Graphics)
#define InitRastPort(rp) \
    AROS_LC1(BOOL, InitRastPort, \
    AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 33, Graphics)

AROS_LP3(void, Move,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 40, Graphics)
#define Move(rp, x, y) \
    AROS_LC3(void, Move, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 40, Graphics)

AROS_LP1(struct TextFont *, OpenFont,
    AROS_LPA(struct TextAttr *, textAttr, A0),
    struct GfxBase *, GfxBase, 12, Graphics)
#define OpenFont(textAttr) \
    AROS_LC1(struct TextFont *, OpenFont, \
    AROS_LCA(struct TextAttr *, textAttr, A0), \
    struct GfxBase *, GfxBase, 12, Graphics)

AROS_LP3(void, PolyDraw,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , count, D0),
    AROS_LPA(WORD            *, polyTable, A0),
    struct GfxBase *, GfxBase, 56, Graphics)
#define PolyDraw(rp, count, polyTable) \
    AROS_LC3(void, PolyDraw, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , count, D0), \
    AROS_LCA(WORD            *, polyTable, A0), \
    struct GfxBase *, GfxBase, 56, Graphics)

AROS_LP3(ULONG, ReadPixel,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 53, Graphics)
#define ReadPixel(rp, x, y) \
    AROS_LC3(ULONG, ReadPixel, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 53, Graphics)

AROS_LP5(void, RectFill,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , xMin, D0),
    AROS_LPA(LONG             , yMin, D1),
    AROS_LPA(LONG             , xMax, D2),
    AROS_LPA(LONG             , yMax, D3),
    struct GfxBase *, GfxBase, 51, Graphics)
#define RectFill(rp, xMin, yMin, xMax, yMax) \
    AROS_LC5(void, RectFill, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , xMin, D0), \
    AROS_LCA(LONG             , yMin, D1), \
    AROS_LCA(LONG             , xMax, D2), \
    AROS_LCA(LONG             , yMax, D3), \
    struct GfxBase *, GfxBase, 51, Graphics)

AROS_LP7(void, ScrollRaster,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , dx, D0),
    AROS_LPA(LONG             , dy, D1),
    AROS_LPA(LONG             , xMin, D2),
    AROS_LPA(LONG             , yMin, D3),
    AROS_LPA(LONG             , xMax, D4),
    AROS_LPA(LONG             , yMax, D5),
    struct GfxBase *, GfxBase, 66, Graphics)
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

AROS_LP4(void, SetABPenDrMd,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , apen, D0),
    AROS_LPA(ULONG            , bpen, D1),
    AROS_LPA(ULONG            , drawMode, D2),
    struct GfxBase *, GfxBase, 149, Graphics)
#define SetABPenDrMd(rp, apen, bpen, drawMode) \
    AROS_LC4(void, SetABPenDrMd, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , apen, D0), \
    AROS_LCA(ULONG            , bpen, D1), \
    AROS_LCA(ULONG            , drawMode, D2), \
    struct GfxBase *, GfxBase, 149, Graphics)

AROS_LP2(void, SetAPen,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , pen, D0),
    struct GfxBase *, GfxBase, 57, Graphics)
#define SetAPen(rp, pen) \
    AROS_LC2(void, SetAPen, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , pen, D0), \
    struct GfxBase *, GfxBase, 57, Graphics)

AROS_LP2(void, SetBPen,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , pen, D0),
    struct GfxBase *, GfxBase, 58, Graphics)
#define SetBPen(rp, pen) \
    AROS_LC2(void, SetBPen, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , pen, D0), \
    struct GfxBase *, GfxBase, 58, Graphics)

AROS_LP2(void, SetDrMd,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , drawMode, D0),
    struct GfxBase *, GfxBase, 59, Graphics)
#define SetDrMd(rp, drawMode) \
    AROS_LC2(void, SetDrMd, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , drawMode, D0), \
    struct GfxBase *, GfxBase, 59, Graphics)

AROS_LP2(void, SetFont,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(struct TextFont *, textFont, A0),
    struct GfxBase *, GfxBase, 11, Graphics)
#define SetFont(rp, textFont) \
    AROS_LC2(void, SetFont, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(struct TextFont *, textFont, A0), \
    struct GfxBase *, GfxBase, 11, Graphics)

AROS_LP2(ULONG, SetOutlinePen,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(ULONG,             pen, D0),
    struct GfxBase *, GfxBase, 163, Graphics)
#define SetOutlinePen(rp, pen) \
    AROS_LC2(ULONG, SetOutlinePen, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(ULONG,             pen, D0), \
    struct GfxBase *, GfxBase, 163, Graphics)

AROS_LP2(void, SetRast,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(ULONG            , pen, D0),
    struct GfxBase *, GfxBase, 39, Graphics)
#define SetRast(rp, pen) \
    AROS_LC2(void, SetRast, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(ULONG            , pen, D0), \
    struct GfxBase *, GfxBase, 39, Graphics)

AROS_LP2(void, SetRPAttrsA,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(struct TagItem  *, tags, A1),
    struct GfxBase *, GfxBase, 173, Graphics)
#define SetRPAttrsA(rp, tags) \
    AROS_LC2(void, SetRPAttrsA, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct TagItem  *, tags, A1), \
    struct GfxBase *, GfxBase, 173, Graphics)

AROS_LP2(ULONG, SetWriteMask,
    AROS_LPA(struct RastPort *, rp,   A0),
    AROS_LPA(ULONG            , mask, D0),
    struct GfxBase *, GfxBase, 164, Graphics)
#define SetWriteMask(rp, mask) \
    AROS_LC2(ULONG, SetWriteMask, \
    AROS_LCA(struct RastPort *, rp,   A0), \
    AROS_LCA(ULONG            , mask, D0), \
    struct GfxBase *, GfxBase, 164, Graphics)

AROS_LP3(void, Text,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(STRPTR           , string, A0),
    AROS_LPA(ULONG            , count, D0),
    struct GfxBase *, GfxBase, 10, Graphics)
#define Text(rp, string, count) \
    AROS_LC3(void, Text, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(STRPTR           , string, A0), \
    AROS_LCA(ULONG            , count, D0), \
    struct GfxBase *, GfxBase, 10, Graphics)

AROS_LP3(WORD, TextLength,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(STRPTR           , string, A0),
    AROS_LPA(ULONG            , count, D0),
    struct GfxBase *, GfxBase, 9, Graphics)
#define TextLength(rp, string, count) \
    AROS_LC3(WORD, TextLength, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(STRPTR           , string, A0), \
    AROS_LCA(ULONG            , count, D0), \
    struct GfxBase *, GfxBase, 9, Graphics)

AROS_LP3(LONG, WritePixel,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 54, Graphics)
#define WritePixel(rp, x, y) \
    AROS_LC3(LONG, WritePixel, \
    AROS_LCA(struct RastPort *, rp, A1), \
    AROS_LCA(LONG             , x, D0), \
    AROS_LCA(LONG             , y, D1), \
    struct GfxBase *, GfxBase, 54, Graphics)


#endif /* CLIB_GRAPHICS_PROTOS_H */
