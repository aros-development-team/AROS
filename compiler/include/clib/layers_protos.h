#ifndef CLIB_GADTOOLS_PROTOS_H
#define CLIB_GADTOOLS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for layers.library
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

AROS_LP1(LONG, BeginUpdate,
    AROS_LPA(struct Layer *, l, A0),
    struct LayersBase *, LayersBase, 13, Layers)

AROS_LP2(LONG, BehindLayer,
    AROS_LPA(LONG          , dummy, A0),
    AROS_LPA(struct Layer *, layer, A1),
    struct LayersBase *, LayersBase, 9, Layers)

AROS_LP9(struct Layer *, CreateBehindHookLayer,
    AROS_LPA(struct Layer_Info *, li, A0),
    AROS_LPA(struct BitMap     *, bm, A1),
    AROS_LPA(LONG               , x0, D0),
    AROS_LPA(LONG               , y0, D1),
    AROS_LPA(LONG               , x1, D2),
    AROS_LPA(LONG               , y1, D3),
    AROS_LPA(LONG               , flags, D4),
    AROS_LPA(struct Hook       *, hook, A3),
    AROS_LPA(struct BitMap     *, bm2, A2),
    struct LayersBase *, LayersBase, 32, Layers)

AROS_LP8(struct Layer *, CreateBehindLayer,
    AROS_LPA(struct Layer_Info *, li, A0),
    AROS_LPA(struct BitMap     *, bm, A1),
    AROS_LPA(LONG               , x0, D0),
    AROS_LPA(LONG               , y0, D1),
    AROS_LPA(LONG               , x1, D2),
    AROS_LPA(LONG               , y1, D3),
    AROS_LPA(LONG               , flags, D4),
    AROS_LPA(struct BitMap     *, bm2, A2),
    struct LayersBase *, LayersBase, 7, Layers)

AROS_LP9(struct Layer *, CreateUpfrontHookLayer,
    AROS_LPA(struct Layer_Info *, li, A0),
    AROS_LPA(struct BitMap     *, bm, A1),
    AROS_LPA(LONG               , x0, D0),
    AROS_LPA(LONG               , y0, D1),
    AROS_LPA(LONG               , x1, D2),
    AROS_LPA(LONG               , y1, D3),
    AROS_LPA(LONG               , flags, D4),
    AROS_LPA(struct Hook       *, hook, A3),
    AROS_LPA(struct BitMap     *, bm2, A2),
    struct LayersBase *, LayersBase, 31, Layers)

AROS_LP8(struct Layer *, CreateUpfrontLayer,
    AROS_LPA(struct Layer_Info *, li, A0),
    AROS_LPA(struct BitMap     *, bm, A1),
    AROS_LPA(LONG               , x0, D0),
    AROS_LPA(LONG               , y0, D1),
    AROS_LPA(LONG               , x1, D2),
    AROS_LPA(LONG               , y1, D3),
    AROS_LPA(LONG               , flags, D4),
    AROS_LPA(struct BitMap     *, bm2, A2),
    struct LayersBase *, LayersBase, 6, Layers)

AROS_LP2(LONG, DeleteLayer,
    AROS_LPA(LONG          , dummy, A0),
    AROS_LPA(struct Layer *, layer, A1),
    struct LayersBase *, LayersBase, 15, Layers)

AROS_LP1(void, DisposeLayerInfo,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 25, Layers)

AROS_LP3(void, DoHookClipRects,
    AROS_LPA(struct Hook      *, hook,  A0),
    AROS_LPA(struct RastPort  *, rport, A1),
    AROS_LPA(struct Rectangle *, rect,  A2),
    struct LayersBase *, LayersBase, 36, Layers)

AROS_LP2(void, EndUpdate,
    AROS_LPA(struct Layer *, layer, A0),
    AROS_LPA(ULONG         , flag, D0),
    struct LayersBase *, LayersBase, 14, Layers)

AROS_LP1(LONG, FattenLayerInfo,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 26, Layers)

AROS_LP1(void, InitLayers,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 5, Layers)

AROS_LP2(struct Region *, InstallClipRegion,
    AROS_LPA(struct Layer  *, layer, A0),
    AROS_LPA(struct Region *, region, A1),
    struct LayersBase *, LayersBase, 29, Layers)

AROS_LP2(struct Hook *, InstallLayerHook,
    AROS_LPA(struct Layer *, layer, A0),
    AROS_LPA(struct Hook  *, hook, A1),
    struct LayersBase *, LayersBase, 33, Layers)

AROS_LP2(struct Hook *, InstallLayerInfoHook,
    AROS_LPA(struct Layer_Info *, li, A0),
    AROS_LPA(struct Hook       *, hook, A1),
    struct LayersBase *, LayersBase, 34, Layers)

AROS_LP2(void, LockLayer,
    AROS_LPA(LONG          , dummy, A0),
    AROS_LPA(struct Layer *, layer, A1),
    struct LayersBase *, LayersBase, 16, Layers)

AROS_LP1(void, LockLayerInfo,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 20, Layers)

AROS_LP1(void, LockLayers,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 18, Layers)

AROS_LP4(LONG, MoveLayer,
    AROS_LPA(LONG          , dummy, A0),
    AROS_LPA(struct Layer *, layer, A1),
    AROS_LPA(LONG          , dx, D0),
    AROS_LPA(LONG          , dy, D1),
    struct LayersBase *, LayersBase, 10, Layers)

AROS_LP2(LONG, MoveLayerInFrontOf,
    AROS_LPA(struct Layer *, layer_to_move, A0),
    AROS_LPA(struct Layer *, other_layer, A1),
    struct LayersBase *, LayersBase, 28, Layers)

AROS_LP5(LONG, MoveSizeLayer,
    AROS_LPA(struct Layer *, layer, A0),
    AROS_LPA(LONG          , dx, D0),
    AROS_LPA(LONG          , dy, D1),
    AROS_LPA(LONG          , dw, D2),
    AROS_LPA(LONG          , dh, D3),
    struct LayersBase *, LayersBase, 30, Layers)

AROS_LP0(struct Layer_Info *, NewLayerInfo,
    struct LayersBase *, LayersBase, 24, Layers)

AROS_LP4(void, ScrollLayer,
    AROS_LPA(LONG          , dummy, A0),
    AROS_LPA(struct Layer *, layer, A1),
    AROS_LPA(LONG          , dx, D0),
    AROS_LPA(LONG          , dy, D1),
    struct LayersBase *, LayersBase, 12, Layers)

AROS_LP4(LONG, SizeLayer,
    AROS_LPA(LONG          , dummy, A0),
    AROS_LPA(struct Layer *, layer, A1),
    AROS_LPA(LONG          , dx, D0),
    AROS_LPA(LONG          , dy, D1),
    struct LayersBase *, LayersBase, 11, Layers)

AROS_LP3(void, SortLayerCR,
    AROS_LPA(struct Layer *, layer, A0),
    AROS_LPA(LONG          , dx,    D0),
    AROS_LPA(LONG          , dy,    D1),
    struct LayersBase *, LayersBase, 35, Layers)

AROS_LP2(void, SwapBitsRastPortClipRect,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(struct ClipRect *, cr, A1),
    struct LayersBase *, LayersBase, 21, Layers)

AROS_LP1(void, ThinLayerInfo,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 27, Layers)

AROS_LP1(void, UnlockLayer,
    AROS_LPA(struct Layer *, layer, A0),
    struct LayersBase *, LayersBase, 17, Layers)

AROS_LP1(void, UnlockLayerInfo,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 23, Layers)

AROS_LP1(void, UnlockLayers,
    AROS_LPA(struct Layer_Info *, li, A0),
    struct LayersBase *, LayersBase, 19, Layers)

AROS_LP2(LONG, UpfrontLayer,
    AROS_LPA(LONG          , dummy, A0),
    AROS_LPA(struct Layer *, layer, A1),
    struct LayersBase *, LayersBase, 8, Layers)

AROS_LP3(struct Layer *, WhichLayer,
    AROS_LPA(struct Layer_Info *, li, A0),
    AROS_LPA(LONG               , x,  D0),
    AROS_LPA(LONG               , y,  D1),
    struct LayersBase *, LayersBase, 22, Layers)

#endif /* CLIB_GADTOOLS_PROTOS_H */
