/*
        (C) 1995-96 AROS - The Amiga Replacement OS
        *** Automatic generated file. Do not edit ***
        Desc: Function table for Layers
        Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,Layers) (void);
void AROS_SLIB_ENTRY(close,Layers) (void);
void AROS_SLIB_ENTRY(expunge,Layers) (void);
void AROS_SLIB_ENTRY(null,Layers) (void);
void AROS_SLIB_ENTRY(InitLayers,Layers) (void);
void AROS_SLIB_ENTRY(CreateUpfrontLayer,Layers) (void);
void AROS_SLIB_ENTRY(CreateBehindLayer,Layers) (void);
void AROS_SLIB_ENTRY(UpfrontLayer,Layers) (void);
void AROS_SLIB_ENTRY(BehindLayer,Layers) (void);
void AROS_SLIB_ENTRY(MoveLayer,Layers) (void);
void AROS_SLIB_ENTRY(SizeLayer,Layers) (void);
void AROS_SLIB_ENTRY(ScrollLayer,Layers) (void);
void AROS_SLIB_ENTRY(BeginUpdate,Layers) (void);
void AROS_SLIB_ENTRY(EndUpdate,Layers) (void);
void AROS_SLIB_ENTRY(DeleteLayer,Layers) (void);
void AROS_SLIB_ENTRY(LockLayer,Layers) (void);
void AROS_SLIB_ENTRY(UnlockLayer,Layers) (void);
void AROS_SLIB_ENTRY(LockLayers,Layers) (void);
void AROS_SLIB_ENTRY(UnlockLayers,Layers) (void);
void AROS_SLIB_ENTRY(LockLayerInfo,Layers) (void);
void AROS_SLIB_ENTRY(SwapBitsRastPortClipRect,Layers) (void);
void AROS_SLIB_ENTRY(WhichLayer,Layers) (void);
void AROS_SLIB_ENTRY(UnlockLayerInfo,Layers) (void);
void AROS_SLIB_ENTRY(NewLayerInfo,Layers) (void);
void AROS_SLIB_ENTRY(DisposeLayerInfo,Layers) (void);
void AROS_SLIB_ENTRY(FattenLayerInfo,Layers) (void);
void AROS_SLIB_ENTRY(ThinLayerInfo,Layers) (void);
void AROS_SLIB_ENTRY(MoveLayerInFrontOf,Layers) (void);
void AROS_SLIB_ENTRY(InstallClipRegion,Layers) (void);
void AROS_SLIB_ENTRY(MoveSizeLayer,Layers) (void);
void AROS_SLIB_ENTRY(CreateUpfrontHookLayer,Layers) (void);
void AROS_SLIB_ENTRY(CreateBehindHookLayer,Layers) (void);
void AROS_SLIB_ENTRY(InstallLayerHook,Layers) (void);
void AROS_SLIB_ENTRY(InstallLayerInfoHook,Layers) (void);
void AROS_SLIB_ENTRY(SortLayerCR,Layers) (void);
void AROS_SLIB_ENTRY(DoHookClipRects,Layers) (void);

void *const Layers_functable[]=
{
    AROS_SLIB_ENTRY(open,Layers), /* 1 */
    AROS_SLIB_ENTRY(close,Layers), /* 2 */
    AROS_SLIB_ENTRY(expunge,Layers), /* 3 */
    AROS_SLIB_ENTRY(null,Layers), /* 4 */
    AROS_SLIB_ENTRY(InitLayers,Layers), /* 5 */
    AROS_SLIB_ENTRY(CreateUpfrontLayer,Layers), /* 6 */
    AROS_SLIB_ENTRY(CreateBehindLayer,Layers), /* 7 */
    AROS_SLIB_ENTRY(UpfrontLayer,Layers), /* 8 */
    AROS_SLIB_ENTRY(BehindLayer,Layers), /* 9 */
    AROS_SLIB_ENTRY(MoveLayer,Layers), /* 10 */
    AROS_SLIB_ENTRY(SizeLayer,Layers), /* 11 */
    AROS_SLIB_ENTRY(ScrollLayer,Layers), /* 12 */
    AROS_SLIB_ENTRY(BeginUpdate,Layers), /* 13 */
    AROS_SLIB_ENTRY(EndUpdate,Layers), /* 14 */
    AROS_SLIB_ENTRY(DeleteLayer,Layers), /* 15 */
    AROS_SLIB_ENTRY(LockLayer,Layers), /* 16 */
    AROS_SLIB_ENTRY(UnlockLayer,Layers), /* 17 */
    AROS_SLIB_ENTRY(LockLayers,Layers), /* 18 */
    AROS_SLIB_ENTRY(UnlockLayers,Layers), /* 19 */
    AROS_SLIB_ENTRY(LockLayerInfo,Layers), /* 20 */
    AROS_SLIB_ENTRY(SwapBitsRastPortClipRect,Layers), /* 21 */
    AROS_SLIB_ENTRY(WhichLayer,Layers), /* 22 */
    AROS_SLIB_ENTRY(UnlockLayerInfo,Layers), /* 23 */
    AROS_SLIB_ENTRY(NewLayerInfo,Layers), /* 24 */
    AROS_SLIB_ENTRY(DisposeLayerInfo,Layers), /* 25 */
    AROS_SLIB_ENTRY(FattenLayerInfo,Layers), /* 26 */
    AROS_SLIB_ENTRY(ThinLayerInfo,Layers), /* 27 */
    AROS_SLIB_ENTRY(MoveLayerInFrontOf,Layers), /* 28 */
    AROS_SLIB_ENTRY(InstallClipRegion,Layers), /* 29 */
    AROS_SLIB_ENTRY(MoveSizeLayer,Layers), /* 30 */
    AROS_SLIB_ENTRY(CreateUpfrontHookLayer,Layers), /* 31 */
    AROS_SLIB_ENTRY(CreateBehindHookLayer,Layers), /* 32 */
    AROS_SLIB_ENTRY(InstallLayerHook,Layers), /* 33 */
    AROS_SLIB_ENTRY(InstallLayerInfoHook,Layers), /* 34 */
    AROS_SLIB_ENTRY(SortLayerCR,Layers), /* 35 */
    AROS_SLIB_ENTRY(DoHookClipRects,Layers), /* 36 */
    (void *)-1L
};
