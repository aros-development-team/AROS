#ifndef CLIB_GRAPHICS_PROTOS_H
#define CLIB_GRAPHICS_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
__AROS_LP1(void, CloseFont,
    __AROS_LPA(struct TextFont *, textFont, A1),
    struct GfxBase *, GfxBase, 13, Graphics)
#define CloseFont(textFont) \
    __AROS_LC1(void, CloseFont, \
    __AROS_LCA(struct TextFont *, textFont, A1), \
    struct GfxBase *, GfxBase, 13, Graphics)

__AROS_LP3(void, Draw,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , x, D0),
    __AROS_LPA(long             , y, D1),
    struct GfxBase *, GfxBase, 41, Graphics)
#define Draw(rp, x, y) \
    __AROS_LC3(void, Draw, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , x, D0), \
    __AROS_LCA(long             , y, D1), \
    struct GfxBase *, GfxBase, 41, Graphics)

__AROS_LP5(void, DrawEllipse,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , xCenter, D0),
    __AROS_LPA(long             , yCenter, D1),
    __AROS_LPA(long             , a, D2),
    __AROS_LPA(long             , b, D3),
    struct GfxBase *, GfxBase, 30, Graphics)
#define DrawEllipse(rp, xCenter, yCenter, a, b) \
    __AROS_LC5(void, DrawEllipse, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , xCenter, D0), \
    __AROS_LCA(long             , yCenter, D1), \
    __AROS_LCA(long             , a, D2), \
    __AROS_LCA(long             , b, D3), \
    struct GfxBase *, GfxBase, 30, Graphics)

__AROS_LP5(void, EraseRect,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , xMin, D0),
    __AROS_LPA(long             , yMin, D1),
    __AROS_LPA(long             , xMax, D2),
    __AROS_LPA(long             , yMax, D3),
    struct GfxBase *, GfxBase, 135, Graphics)
#define EraseRect(rp, xMin, yMin, xMax, yMax) \
    __AROS_LC5(void, EraseRect, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , xMin, D0), \
    __AROS_LCA(long             , yMin, D1), \
    __AROS_LCA(long             , xMax, D2), \
    __AROS_LCA(long             , yMax, D3), \
    struct GfxBase *, GfxBase, 135, Graphics)

__AROS_LP1(ULONG, GetAPen,
    __AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 143, Graphics)
#define GetAPen(rp) \
    __AROS_LC1(ULONG, GetAPen, \
    __AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 143, Graphics)

__AROS_LP1(ULONG, GetBPen,
    __AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 144, Graphics)
#define GetBPen(rp) \
    __AROS_LC1(ULONG, GetBPen, \
    __AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 144, Graphics)

__AROS_LP1(ULONG, GetDrMd,
    __AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 145, Graphics)
#define GetDrMd(rp) \
    __AROS_LC1(ULONG, GetDrMd, \
    __AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 145, Graphics)

__AROS_LP1(ULONG, GetOutlinePen,
    __AROS_LPA(struct RastPort *, rp, A0),
    struct GfxBase *, GfxBase, 146, Graphics)
#define GetOutlinePen(rp) \
    __AROS_LC1(ULONG, GetOutlinePen, \
    __AROS_LCA(struct RastPort *, rp, A0), \
    struct GfxBase *, GfxBase, 146, Graphics)

__AROS_LP1(void, InitRastPort,
    __AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 33, Graphics)
#define InitRastPort(rp) \
    __AROS_LC1(void, InitRastPort, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    struct GfxBase *, GfxBase, 33, Graphics)

__AROS_LP3(void, Move,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , x, D0),
    __AROS_LPA(long             , y, D1),
    struct GfxBase *, GfxBase, 40, Graphics)
#define Move(rp, x, y) \
    __AROS_LC3(void, Move, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , x, D0), \
    __AROS_LCA(long             , y, D1), \
    struct GfxBase *, GfxBase, 40, Graphics)

__AROS_LP1(struct TextFont *, OpenFont,
    __AROS_LPA(struct TextAttr *, textAttr, A0),
    struct GfxBase *, GfxBase, 12, Graphics)
#define OpenFont(textAttr) \
    __AROS_LC1(struct TextFont *, OpenFont, \
    __AROS_LCA(struct TextAttr *, textAttr, A0), \
    struct GfxBase *, GfxBase, 12, Graphics)

__AROS_LP3(void, PolyDraw,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , count, D0),
    __AROS_LPA(WORD            *, polyTable, A0),
    struct GfxBase *, GfxBase, 56, Graphics)
#define PolyDraw(rp, count, polyTable) \
    __AROS_LC3(void, PolyDraw, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , count, D0), \
    __AROS_LCA(WORD            *, polyTable, A0), \
    struct GfxBase *, GfxBase, 56, Graphics)

__AROS_LP3(ULONG, ReadPixel,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , x, D0),
    __AROS_LPA(long             , y, D1),
    struct GfxBase *, GfxBase, 53, Graphics)
#define ReadPixel(rp, x, y) \
    __AROS_LC3(ULONG, ReadPixel, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , x, D0), \
    __AROS_LCA(long             , y, D1), \
    struct GfxBase *, GfxBase, 53, Graphics)

__AROS_LP5(void, RectFill,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , xMin, D0),
    __AROS_LPA(long             , yMin, D1),
    __AROS_LPA(long             , xMax, D2),
    __AROS_LPA(long             , yMax, D3),
    struct GfxBase *, GfxBase, 51, Graphics)
#define RectFill(rp, xMin, yMin, xMax, yMax) \
    __AROS_LC5(void, RectFill, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , xMin, D0), \
    __AROS_LCA(long             , yMin, D1), \
    __AROS_LCA(long             , xMax, D2), \
    __AROS_LCA(long             , yMax, D3), \
    struct GfxBase *, GfxBase, 51, Graphics)

__AROS_LP7(void, ScrollRaster,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , dx, D0),
    __AROS_LPA(long             , dy, D1),
    __AROS_LPA(long             , xMin, D2),
    __AROS_LPA(long             , yMin, D3),
    __AROS_LPA(long             , xMax, D4),
    __AROS_LPA(long             , yMax, D5),
    struct GfxBase *, GfxBase, 66, Graphics)
#define ScrollRaster(rp, dx, dy, xMin, yMin, xMax, yMax) \
    __AROS_LC7(void, ScrollRaster, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , dx, D0), \
    __AROS_LCA(long             , dy, D1), \
    __AROS_LCA(long             , xMin, D2), \
    __AROS_LCA(long             , yMin, D3), \
    __AROS_LCA(long             , xMax, D4), \
    __AROS_LCA(long             , yMax, D5), \
    struct GfxBase *, GfxBase, 66, Graphics)

__AROS_LP2(void, SetAPen,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(unsigned long    , pen, D0),
    struct GfxBase *, GfxBase, 57, Graphics)
#define SetAPen(rp, pen) \
    __AROS_LC2(void, SetAPen, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(unsigned long    , pen, D0), \
    struct GfxBase *, GfxBase, 57, Graphics)

__AROS_LP2(void, SetBPen,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(unsigned long    , pen, D0),
    struct GfxBase *, GfxBase, 58, Graphics)
#define SetBPen(rp, pen) \
    __AROS_LC2(void, SetBPen, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(unsigned long    , pen, D0), \
    struct GfxBase *, GfxBase, 58, Graphics)

__AROS_LP2(void, SetDrMd,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(unsigned long    , drawMode, D0),
    struct GfxBase *, GfxBase, 59, Graphics)
#define SetDrMd(rp, drawMode) \
    __AROS_LC2(void, SetDrMd, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(unsigned long    , drawMode, D0), \
    struct GfxBase *, GfxBase, 59, Graphics)

__AROS_LP2(LONG, SetFont,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(struct TextFont *, textFont, A0),
    struct GfxBase *, GfxBase, 11, Graphics)
#define SetFont(rp, textFont) \
    __AROS_LC2(LONG, SetFont, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(struct TextFont *, textFont, A0), \
    struct GfxBase *, GfxBase, 11, Graphics)

__AROS_LP2(ULONG, SetOutlinePen,
    __AROS_LPA(struct RastPort *, rp, A0),
    __AROS_LPA(ULONG,             pen, D0),
    struct GfxBase *, GfxBase, 163, Graphics)
#define SetOutlinePen(rp, pen) \
    __AROS_LC2(ULONG, SetOutlinePen, \
    __AROS_LCA(struct RastPort *, rp, A0), \
    __AROS_LCA(ULONG,             pen, D0), \
    struct GfxBase *, GfxBase, 163, Graphics)

__AROS_LP2(void, SetRast,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(unsigned long    , pen, D0),
    struct GfxBase *, GfxBase, 39, Graphics)
#define SetRast(rp, pen) \
    __AROS_LC2(void, SetRast, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(unsigned long    , pen, D0), \
    struct GfxBase *, GfxBase, 39, Graphics)

__AROS_LP3(LONG, Text,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(STRPTR           , string, A0),
    __AROS_LPA(unsigned long    , count, D0),
    struct GfxBase *, GfxBase, 10, Graphics)
#define Text(rp, string, count) \
    __AROS_LC3(LONG, Text, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(STRPTR           , string, A0), \
    __AROS_LCA(unsigned long    , count, D0), \
    struct GfxBase *, GfxBase, 10, Graphics)

__AROS_LP3(WORD, TextLength,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(STRPTR           , string, A0),
    __AROS_LPA(unsigned long    , count, D0),
    struct GfxBase *, GfxBase, 9, Graphics)
#define TextLength(rp, string, count) \
    __AROS_LC3(WORD, TextLength, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(STRPTR           , string, A0), \
    __AROS_LCA(unsigned long    , count, D0), \
    struct GfxBase *, GfxBase, 9, Graphics)

__AROS_LP3(LONG, WritePixel,
    __AROS_LPA(struct RastPort *, rp, A1),
    __AROS_LPA(long             , x, D0),
    __AROS_LPA(long             , y, D1),
    struct GfxBase *, GfxBase, 54, Graphics)
#define WritePixel(rp, x, y) \
    __AROS_LC3(LONG, WritePixel, \
    __AROS_LCA(struct RastPort *, rp, A1), \
    __AROS_LCA(long             , x, D0), \
    __AROS_LCA(long             , y, D1), \
    struct GfxBase *, GfxBase, 54, Graphics)


#endif /* CLIB_GRAPHICS_PROTOS_H */
