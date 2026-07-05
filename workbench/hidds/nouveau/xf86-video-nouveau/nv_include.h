#ifndef __NV_INCLUDE_H__
#define __NV_INCLUDE_H__
/*
    Copyright (C) 2011-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <math.h>
#include <aros/debug.h>

#include "nouveau_intern.h"

/* Some overriding defines for AROS */
#define Bool                        BOOL
#define ScrnInfoPtr                 struct CardData *
#define ScreenPtr                   struct CardData *
#define NVPTR(x)                    (x)
#define xf86ScreenToScrn(x)         (x)
#define NVPtr                       struct CardData *
#define PixmapPtr                   struct HIDDNouveauBitMapData *
#define xf86DrvMsg(a, b, fmt, ...)  bug(fmt, ##__VA_ARGS__)
#define ErrorF(msg, ...)            bug(msg, ##__VA_ARGS__)
#define PictFormatShort             LONG
#define PictTransformPtr            APTR
#define Pixel                       HIDDT_Pixel
#define CARD32                      LONG
#define BoxPtr                      APTR

#include "nouveau_local.h"


#define nouveau_pixmap_bo(x)    (x->bo)
#define exaGetPixmapPitch(x)    (x->pitch)

#define PictOpSaturate          14
#define GXcopy                  0x03
#define EXA_PM_IS_SOLID(a, b)   1

void NVXVComputeBicubicFilter(struct nouveau_bo *, unsigned, unsigned);

Bool NV04EXAUploadIFC(ScrnInfoPtr, const char *src, int src_pitch,
             PixmapPtr pdPix, int x, int y, int w, int h, int cpp);
Bool NV04EXARectM2MF(NVPtr pNv, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int);

Bool NVAccelInitNV40TCL(ScrnInfoPtr pScrn);
Bool NVAccelInitNV30TCL(ScrnInfoPtr pScrn);
Bool NVAccelInitNV10TCL(ScrnInfoPtr pScrn);

Bool NVAccelInitM2MF_NV50(ScrnInfoPtr pScrn);
Bool NVAccelInit2D_NV50(ScrnInfoPtr pScrn);
Bool NVAccelInitNV50TCL(ScrnInfoPtr pScrn);
Bool NV50EXAUploadSIFC(const char *src, int src_pitch,
             PixmapPtr pdPix, int x, int y, int w, int h, int cpp);
Bool NV50EXARectM2MF(NVPtr pNv, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int);

Bool NVAccelInitM2MF_NVC0(ScrnInfoPtr pScrn);
Bool NVAccelInit2D_NVC0(ScrnInfoPtr pScrn);
Bool NVAccelInit3D_NVC0(ScrnInfoPtr pScrn);
Bool NVC0EXAUploadSIFC(const char *src, int src_pitch,
             PixmapPtr pdPix, int x, int y, int w, int h, int cpp);
Bool NVC0EXARectM2MF(NVPtr pNv, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int);

Bool NVAccelInitP2MF_NVE0(ScrnInfoPtr pScrn);
Bool NVAccelInitCOPY_NVE0(ScrnInfoPtr pScrn);
Bool NVE0EXARectCopy(NVPtr pNv, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int,
             struct nouveau_bo *, uint32_t, int, int, int, int, int);

Bool nv50_style_tiled_pixmap(PixmapPtr ppix);


#define NOUVEAU_CREATE_PIXMAP_ZETA      0x10000000
#define NOUVEAU_CREATE_PIXMAP_TILED     0x20000000
#define NOUVEAU_CREATE_PIXMAP_SCANOUT   0x40000000

struct Picture
{
    LONG format;
    BOOL componentAlpha;
    LONG filter;
    BOOL repeat;
    LONG repeatType;

    struct
    {
        ULONG width;
        ULONG height;
    } *pDrawable, drawableAREA;
};

typedef struct Picture * PicturePtr;

#define PictFilterNearest   1
#define PictFilterBilinear  2

#define RepeatNone          0 /* This must be zero/FALSE, see nv10_exa for usage against ppict->repeat which is BOOL */
#define RepeatNormal        1
#define RepeatReflect       2
#define RepeatPad           3

#define PICT_UNKNOWN        0
#define PICT_a8r8g8b8       1
#define PICT_x8r8g8b8       2
#define PICT_a8b8g8r8       3
#define PICT_x8b8g8r8       4
#define PICT_b8g8r8a8       5
#define PICT_b8g8r8x8       6
#define PICT_a2b10g10r10    7
#define PICT_x2b10g10r10    8
#define PICT_a2r10g10b10    9
#define PICT_x2r10g10b10    10
#define PICT_a1r5g5b5       11
#define PICT_x1r5g5b5       12
#define PICT_a1b5g5r5       13
#define PICT_x1b5g5r5       14
#define PICT_x4r4g4b4       15
#define PICT_a4r4g4b4       16
#define PICT_x4b4g4r4       17
#define PICT_a4b4g4r4       18
#define PICT_r5g6b5         19
#define PICT_b5g6r5         20
#define PICT_a8             21

static inline BOOL PICT_FORMAT_A(int format)
{
    switch(format)
    {
    case(PICT_a8r8g8b8):
    case(PICT_a8b8g8r8):
    case(PICT_b8g8r8a8):
    case(PICT_a2b10g10r10):
    case(PICT_a2r10g10b10):
    case(PICT_a1r5g5b5):
    case(PICT_a1b5g5r5):
    case(PICT_a4r4g4b4):
    case(PICT_a4b4g4r4):
    case(PICT_a8):
        return TRUE;
    }

    return FALSE;
}

static inline BOOL PICT_FORMAT_RGB(int format)
{
    switch(format)
    {
    case(PICT_a8r8g8b8):
    case(PICT_x8r8g8b8):
    case(PICT_x8b8g8r8):
    case(PICT_a8b8g8r8):
    case(PICT_b8g8r8a8):
    case(PICT_b8g8r8x8):
    case(PICT_a2b10g10r10):
    case(PICT_x2b10g10r10):
    case(PICT_a2r10g10b10):
    case(PICT_x2r10g10b10):
    case(PICT_a1r5g5b5):
    case(PICT_x1r5g5b5):
    case(PICT_a1b5g5r5):
    case(PICT_x1b5g5r5):
    case(PICT_x4r4g4b4):
    case(PICT_a4r4g4b4):
    case(PICT_x4b4g4r4):
    case(PICT_a4b4g4r4):
    case(PICT_r5g6b5):
    case(PICT_b5g6r5):
        return TRUE;
    }

    return FALSE;
}


static inline VOID HIDDNouveauFillPictureFromBitMapData(struct Picture * pPict, 
    struct HIDDNouveauBitMapData * bmdata)
{
    /* pPict->format */
    if (bmdata->drawable.depth == 32)
        pPict->format = PICT_a8r8g8b8;
    else if (bmdata->drawable.depth == 24)
        pPict->format = PICT_x8r8g8b8;
    else if (bmdata->drawable.depth == 16)
        pPict->format = PICT_r5g6b5;
    else
        pPict->format = PICT_UNKNOWN;

    /* pPict->componentAlpha - keep this always as FALSE, used when mask
       bitmap would be present (which is not the case in AROS */
    pPict->componentAlpha = FALSE;
    
    /* pPict->filter - keep this always as PictFilterNearest, unless you want
       bi-linear (probably slower and might give weird effects */
    pPict->filter = PictFilterNearest;
    
    /* pPict->repeat - keep this always as FALSE */
    pPict->repeat = FALSE;
    /* pPict->repeatType - value does not matter as long as repeat is FALSE */
    pPict->repeatType = RepeatNone;

    /* pPict->pDrawable - copy width and height to minize source code changes */
    pPict->pDrawable = &pPict->drawableAREA;
    pPict->pDrawable->width = bmdata->drawable.width;
    pPict->pDrawable->height = bmdata->drawable.height;
}

#endif /* __NV_INCLUDE_H__ */
