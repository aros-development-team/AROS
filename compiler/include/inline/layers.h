#ifndef _INLINE_LAYERS_H
#define _INLINE_LAYERS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef LAYERS_BASE_NAME
#define LAYERS_BASE_NAME LayersBase
#endif

#define BeginUpdate(l) \
	LP1(0x4e, LONG, BeginUpdate, struct Layer *, l, a0, \
	, LAYERS_BASE_NAME)

#define BehindLayer(dummy, layer) \
	LP2(0x36, LONG, BehindLayer, long, dummy, a0, struct Layer *, layer, a1, \
	, LAYERS_BASE_NAME)

#define CreateBehindHookLayer(li, bm, x0, y0, x1, y1, flags, hook, bm2) \
	LP9(0xc0, struct Layer *, CreateBehindHookLayer, struct Layer_Info *, li, a0, struct BitMap *, bm, a1, long, x0, d0, long, y0, d1, long, x1, d2, long, y1, d3, long, flags, d4, struct Hook *, hook, a3, struct BitMap *, bm2, a2, \
	, LAYERS_BASE_NAME)

#define CreateBehindLayer(li, bm, x0, y0, x1, y1, flags, bm2) \
	LP8(0x2a, struct Layer *, CreateBehindLayer, struct Layer_Info *, li, a0, struct BitMap *, bm, a1, long, x0, d0, long, y0, d1, long, x1, d2, long, y1, d3, long, flags, d4, struct BitMap *, bm2, a2, \
	, LAYERS_BASE_NAME)

#define CreateUpfrontHookLayer(li, bm, x0, y0, x1, y1, flags, hook, bm2) \
	LP9(0xba, struct Layer *, CreateUpfrontHookLayer, struct Layer_Info *, li, a0, struct BitMap *, bm, a1, long, x0, d0, long, y0, d1, long, x1, d2, long, y1, d3, long, flags, d4, struct Hook *, hook, a3, struct BitMap *, bm2, a2, \
	, LAYERS_BASE_NAME)

#define CreateUpfrontLayer(li, bm, x0, y0, x1, y1, flags, bm2) \
	LP8(0x24, struct Layer *, CreateUpfrontLayer, struct Layer_Info *, li, a0, struct BitMap *, bm, a1, long, x0, d0, long, y0, d1, long, x1, d2, long, y1, d3, long, flags, d4, struct BitMap *, bm2, a2, \
	, LAYERS_BASE_NAME)

#define DeleteLayer(dummy, layer) \
	LP2(0x5a, LONG, DeleteLayer, long, dummy, a0, struct Layer *, layer, a1, \
	, LAYERS_BASE_NAME)

#define DisposeLayerInfo(li) \
	LP1NR(0x96, DisposeLayerInfo, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define DoHookClipRects(hook, rport, rect) \
	LP3NR(0xd8, DoHookClipRects, struct Hook *, hook, a0, struct RastPort *, rport, a1, struct Rectangle *, rect, a2, \
	, LAYERS_BASE_NAME)

#define EndUpdate(layer, flag) \
	LP2NR(0x54, EndUpdate, struct Layer *, layer, a0, unsigned long, flag, d0, \
	, LAYERS_BASE_NAME)

#define FattenLayerInfo(li) \
	LP1(0x9c, LONG, FattenLayerInfo, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define InitLayers(li) \
	LP1NR(0x1e, InitLayers, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define InstallClipRegion(layer, region) \
	LP2(0xae, struct Region *, InstallClipRegion, struct Layer *, layer, a0, struct Region *, region, a1, \
	, LAYERS_BASE_NAME)

#define InstallLayerHook(layer, hook) \
	LP2(0xc6, struct Hook *, InstallLayerHook, struct Layer *, layer, a0, struct Hook *, hook, a1, \
	, LAYERS_BASE_NAME)

#define InstallLayerInfoHook(li, hook) \
	LP2(0xcc, struct Hook *, InstallLayerInfoHook, struct Layer_Info *, li, a0, struct Hook *, hook, a1, \
	, LAYERS_BASE_NAME)

#define LockLayer(dummy, layer) \
	LP2NR(0x60, LockLayer, long, dummy, a0, struct Layer *, layer, a1, \
	, LAYERS_BASE_NAME)

#define LockLayerInfo(li) \
	LP1NR(0x78, LockLayerInfo, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define LockLayers(li) \
	LP1NR(0x6c, LockLayers, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define MoveLayer(dummy, layer, dx, dy) \
	LP4(0x3c, LONG, MoveLayer, long, dummy, a0, struct Layer *, layer, a1, long, dx, d0, long, dy, d1, \
	, LAYERS_BASE_NAME)

#define MoveLayerInFrontOf(layer_to_move, other_layer) \
	LP2(0xa8, LONG, MoveLayerInFrontOf, struct Layer *, layer_to_move, a0, struct Layer *, other_layer, a1, \
	, LAYERS_BASE_NAME)

#define MoveSizeLayer(layer, dx, dy, dw, dh) \
	LP5(0xb4, LONG, MoveSizeLayer, struct Layer *, layer, a0, long, dx, d0, long, dy, d1, long, dw, d2, long, dh, d3, \
	, LAYERS_BASE_NAME)

#define NewLayerInfo() \
	LP0(0x90, struct Layer_Info *, NewLayerInfo, \
	, LAYERS_BASE_NAME)

#define ScrollLayer(dummy, layer, dx, dy) \
	LP4NR(0x48, ScrollLayer, long, dummy, a0, struct Layer *, layer, a1, long, dx, d0, long, dy, d1, \
	, LAYERS_BASE_NAME)

#define SizeLayer(dummy, layer, dx, dy) \
	LP4(0x42, LONG, SizeLayer, long, dummy, a0, struct Layer *, layer, a1, long, dx, d0, long, dy, d1, \
	, LAYERS_BASE_NAME)

#define SortLayerCR(layer, dx, dy) \
	LP3NR(0xd2, SortLayerCR, struct Layer *, layer, a0, long, dx, d0, long, dy, d1, \
	, LAYERS_BASE_NAME)

#define SwapBitsRastPortClipRect(rp, cr) \
	LP2NR(0x7e, SwapBitsRastPortClipRect, struct RastPort *, rp, a0, struct ClipRect *, cr, a1, \
	, LAYERS_BASE_NAME)

#define ThinLayerInfo(li) \
	LP1NR(0xa2, ThinLayerInfo, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define UnlockLayer(layer) \
	LP1NR(0x66, UnlockLayer, struct Layer *, layer, a0, \
	, LAYERS_BASE_NAME)

#define UnlockLayerInfo(li) \
	LP1NR(0x8a, UnlockLayerInfo, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define UnlockLayers(li) \
	LP1NR(0x72, UnlockLayers, struct Layer_Info *, li, a0, \
	, LAYERS_BASE_NAME)

#define UpfrontLayer(dummy, layer) \
	LP2(0x30, LONG, UpfrontLayer, long, dummy, a0, struct Layer *, layer, a1, \
	, LAYERS_BASE_NAME)

#define WhichLayer(li, x, y) \
	LP3(0x84, struct Layer *, WhichLayer, struct Layer_Info *, li, a0, long, x, d0, long, y, d1, \
	, LAYERS_BASE_NAME)

#endif /* _INLINE_LAYERS_H */
