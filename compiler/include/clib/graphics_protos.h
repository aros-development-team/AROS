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

AROS_LP1(void, CloseFont,
    AROS_LPA(struct TextFont *, textFont, A1),
    struct GfxBase *, GfxBase, 13, Graphics)

AROS_LP0(struct RastPort *, CreateRastPort,
    struct GfxBase *, GfxBase, 177, Graphics)

AROS_LP1(void, DeinitRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 179, Graphics)

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

AROS_LP1(BOOL, InitRastPort,
    AROS_LPA(struct RastPort *, rp, A1),
    struct GfxBase *, GfxBase, 33, Graphics)

AROS_LP3(void, Move,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 40, Graphics)

AROS_LP1(struct TextFont *, OpenFont,
    AROS_LPA(struct TextAttr *, textAttr, A0),
    struct GfxBase *, GfxBase, 12, Graphics)

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

AROS_LP3(WORD, TextLength,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(STRPTR           , string, A0),
    AROS_LPA(ULONG            , count, D0),
    struct GfxBase *, GfxBase, 9, Graphics)

AROS_LP3(LONG, WritePixel,
    AROS_LPA(struct RastPort *, rp, A1),
    AROS_LPA(LONG             , x, D0),
    AROS_LPA(LONG             , y, D1),
    struct GfxBase *, GfxBase, 54, Graphics)


#endif /* CLIB_GRAPHICS_PROTOS_H */
