#ifndef DEFINES_LAYERS_H
#define DEFINES_LAYERS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define BeginUpdate(l) \
    AROS_LC1(LONG, BeginUpdate, \
    AROS_LCA(struct Layer *, l, A0), \
    struct LayersBase *, LayersBase, 13, Layers)

#define BehindLayer(dummy, layer) \
    AROS_LC2(LONG, BehindLayer, \
    AROS_LCA(LONG          , dummy, A0), \
    AROS_LCA(struct Layer *, layer, A1), \
    struct LayersBase *, LayersBase, 9, Layers)

#define CreateBehindHookLayer(li, bm, x0, y0, x1, y1, flags, hook, bm2) \
    AROS_LC9(struct Layer *, CreateBehindHookLayer, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    AROS_LCA(struct BitMap     *, bm, A1), \
    AROS_LCA(LONG               , x0, D0), \
    AROS_LCA(LONG               , y0, D1), \
    AROS_LCA(LONG               , x1, D2), \
    AROS_LCA(LONG               , y1, D3), \
    AROS_LCA(LONG               , flags, D4), \
    AROS_LCA(struct Hook       *, hook, A3), \
    AROS_LCA(struct BitMap     *, bm2, A2), \
    struct LayersBase *, LayersBase, 32, Layers)

#define CreateBehindLayer(li, bm, x0, y0, x1, y1, flags, bm2) \
    AROS_LC8(struct Layer *, CreateBehindLayer, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    AROS_LCA(struct BitMap     *, bm, A1), \
    AROS_LCA(LONG               , x0, D0), \
    AROS_LCA(LONG               , y0, D1), \
    AROS_LCA(LONG               , x1, D2), \
    AROS_LCA(LONG               , y1, D3), \
    AROS_LCA(LONG               , flags, D4), \
    AROS_LCA(struct BitMap     *, bm2, A2), \
    struct LayersBase *, LayersBase, 7, Layers)

#define CreateUpfrontHookLayer(li, bm, x0, y0, x1, y1, flags, hook, bm2) \
    AROS_LC9(struct Layer *, CreateUpfrontHookLayer, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    AROS_LCA(struct BitMap     *, bm, A1), \
    AROS_LCA(LONG               , x0, D0), \
    AROS_LCA(LONG               , y0, D1), \
    AROS_LCA(LONG               , x1, D2), \
    AROS_LCA(LONG               , y1, D3), \
    AROS_LCA(LONG               , flags, D4), \
    AROS_LCA(struct Hook       *, hook, A3), \
    AROS_LCA(struct BitMap     *, bm2, A2), \
    struct LayersBase *, LayersBase, 31, Layers)

#define CreateUpfrontLayer(li, bm, x0, y0, x1, y1, flags, bm2) \
    AROS_LC8(struct Layer *, CreateUpfrontLayer, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    AROS_LCA(struct BitMap     *, bm, A1), \
    AROS_LCA(LONG               , x0, D0), \
    AROS_LCA(LONG               , y0, D1), \
    AROS_LCA(LONG               , x1, D2), \
    AROS_LCA(LONG               , y1, D3), \
    AROS_LCA(LONG               , flags, D4), \
    AROS_LCA(struct BitMap     *, bm2, A2), \
    struct LayersBase *, LayersBase, 6, Layers)

#define DeleteLayer(dummy, layer) \
    AROS_LC2(LONG, DeleteLayer, \
    AROS_LCA(LONG          , dummy, A0), \
    AROS_LCA(struct Layer *, layer, A1), \
    struct LayersBase *, LayersBase, 15, Layers)

#define DisposeLayerInfo(li) \
    AROS_LC1(void, DisposeLayerInfo, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 25, Layers)

#define DoHookClipRects(hook, rport, rect) \
    AROS_LC3(void, DoHookClipRects, \
    AROS_LCA(struct Hook      *, hook,  A0), \
    AROS_LCA(struct RastPort  *, rport, A1), \
    AROS_LCA(struct Rectangle *, rect,  A2), \
    struct LayersBase *, LayersBase, 36, Layers)

#define EndUpdate(layer, flag) \
    AROS_LC2(void, EndUpdate, \
    AROS_LCA(struct Layer *, layer, A0), \
    AROS_LCA(ULONG         , flag, D0), \
    struct LayersBase *, LayersBase, 14, Layers)

#define FattenLayerInfo(li) \
    AROS_LC1(LONG, FattenLayerInfo, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 26, Layers)

#define InitLayers(li) \
    AROS_LC1(void, InitLayers, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 5, Layers)

#define InstallClipRegion(layer, region) \
    AROS_LC2(struct Region *, InstallClipRegion, \
    AROS_LCA(struct Layer  *, layer, A0), \
    AROS_LCA(struct Region *, region, A1), \
    struct LayersBase *, LayersBase, 29, Layers)

#define InstallLayerHook(layer, hook) \
    AROS_LC2(struct Hook *, InstallLayerHook, \
    AROS_LCA(struct Layer *, layer, A0), \
    AROS_LCA(struct Hook  *, hook, A1), \
    struct LayersBase *, LayersBase, 33, Layers)

#define InstallLayerInfoHook(li, hook) \
    AROS_LC2(struct Hook *, InstallLayerInfoHook, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    AROS_LCA(struct Hook       *, hook, A1), \
    struct LayersBase *, LayersBase, 34, Layers)

#define LockLayer(dummy, layer) \
    AROS_LC2(void, LockLayer, \
    AROS_LCA(LONG          , dummy, A0), \
    AROS_LCA(struct Layer *, layer, A1), \
    struct LayersBase *, LayersBase, 16, Layers)

#define LockLayerInfo(li) \
    AROS_LC1(void, LockLayerInfo, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 20, Layers)

#define LockLayers(li) \
    AROS_LC1(void, LockLayers, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 18, Layers)

#define MoveLayer(dummy, layer, dx, dy) \
    AROS_LC4(LONG, MoveLayer, \
    AROS_LCA(LONG          , dummy, A0), \
    AROS_LCA(struct Layer *, layer, A1), \
    AROS_LCA(LONG          , dx, D0), \
    AROS_LCA(LONG          , dy, D1), \
    struct LayersBase *, LayersBase, 10, Layers)

#define MoveLayerInFrontOf(layer_to_move, other_layer) \
    AROS_LC2(LONG, MoveLayerInFrontOf, \
    AROS_LCA(struct Layer *, layer_to_move, A0), \
    AROS_LCA(struct Layer *, other_layer, A1), \
    struct LayersBase *, LayersBase, 28, Layers)

#define MoveSizeLayer(layer, dx, dy, dw, dh) \
    AROS_LC5(LONG, MoveSizeLayer, \
    AROS_LCA(struct Layer *, layer, A0), \
    AROS_LCA(LONG          , dx, D0), \
    AROS_LCA(LONG          , dy, D1), \
    AROS_LCA(LONG          , dw, D2), \
    AROS_LCA(LONG          , dh, D3), \
    struct LayersBase *, LayersBase, 30, Layers)

#define NewLayerInfo() \
    AROS_LC0(struct Layer_Info *, NewLayerInfo, \
    struct LayersBase *, LayersBase, 24, Layers)

#define ScrollLayer(dummy, layer, dx, dy) \
    AROS_LC4(void, ScrollLayer, \
    AROS_LCA(LONG          , dummy, A0), \
    AROS_LCA(struct Layer *, layer, A1), \
    AROS_LCA(LONG          , dx, D0), \
    AROS_LCA(LONG          , dy, D1), \
    struct LayersBase *, LayersBase, 12, Layers)

#define SizeLayer(dummy, layer, dx, dy) \
    AROS_LC4(LONG, SizeLayer, \
    AROS_LCA(LONG          , dummy, A0), \
    AROS_LCA(struct Layer *, layer, A1), \
    AROS_LCA(LONG          , dx, D0), \
    AROS_LCA(LONG          , dy, D1), \
    struct LayersBase *, LayersBase, 11, Layers)

#define SortLayerCR(layer, dx, dy) \
    AROS_LC3(void, SortLayerCR, \
    AROS_LCA(struct Layer *, layer, A0), \
    AROS_LCA(LONG          , dx,    D0), \
    AROS_LCA(LONG          , dy,    D1), \
    struct LayersBase *, LayersBase, 35, Layers)

#define SwapBitsRastPortClipRect(rp, cr) \
    AROS_LC2(void, SwapBitsRastPortClipRect, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct ClipRect *, cr, A1), \
    struct LayersBase *, LayersBase, 21, Layers)

#define ThinLayerInfo(li) \
    AROS_LC1(void, ThinLayerInfo, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 27, Layers)

#define UnlockLayer(layer) \
    AROS_LC1(void, UnlockLayer, \
    AROS_LCA(struct Layer *, layer, A0), \
    struct LayersBase *, LayersBase, 17, Layers)

#define UnlockLayerInfo(li) \
    AROS_LC1(void, UnlockLayerInfo, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 23, Layers)

#define UnlockLayers(li) \
    AROS_LC1(void, UnlockLayers, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 19, Layers)

#define UpfrontLayer(dummy, layer) \
    AROS_LC2(LONG, UpfrontLayer, \
    AROS_LCA(LONG          , dummy, A0), \
    AROS_LCA(struct Layer *, layer, A1), \
    struct LayersBase *, LayersBase, 8, Layers)

#define WhichLayer(li, x, y) \
    AROS_LC3(struct Layer *, WhichLayer, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    AROS_LCA(LONG               , x,  D0), \
    AROS_LCA(LONG               , y,  D1), \
    struct LayersBase *, LayersBase, 22, Layers)


#endif /* DEFINES_LAYERS_H */
