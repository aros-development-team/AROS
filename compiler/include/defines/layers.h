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

#define FattenLayerInfo(li) \
    AROS_LC1(LONG, FattenLayerInfo, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 26, Layers)

#define InitLayers(li) \
    AROS_LC1(void, InitLayers, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    struct LayersBase *, LayersBase, 5, Layers)

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

#define NewLayerInfo() \
    AROS_LC0(struct Layer_Info *, NewLayerInfo, \
    struct LayersBase *, LayersBase, 24, Layers)

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

#define WhichLayer(li, x, y) \
    AROS_LC3(struct Layer *, WhichLayer, \
    AROS_LCA(struct Layer_Info *, li, A0), \
    AROS_LCA(LONG               , x,  D0), \
    AROS_LCA(LONG               , y,  D1), \
    struct LayersBase *, LayersBase, 22, Layers)


#endif /* DEFINES_LAYERS_H */
