#ifndef __NV_INCLUDE_H__
#define __NV_INCLUDE_H__

#include "nouveau_intern.h"
#include "nouveau_class.h"

/* Some overriding defines for AROS */
#define Bool                        BOOL
#define ScrnInfoPtr                 struct CardData *
#define NVPTR(x)                    x
#define NVPtr                       struct CardData *
#define Architecture                architecture
#define PixmapPtr                   struct HIDDNouveauBitMapData *
#define xf86DrvMsg(a, b, fmt, ...)  bug(fmt, ##__VA_ARGS__)
#define ErrorF(msg)                 bug(msg)
#define PictFormatShort             LONG
#define PictTransformPtr            APTR

struct Picture
{
    LONG format;
    BOOL componentAlpha;
    LONG filter;
    BOOL repeat;
    LONG repeatType;
};

typedef struct Picture * PicturePtr;

#define PictFilterNearest   1
#define PictFilterBilinear  2

#define RepeatNormal        1
#define RepeatReflect       2
#define RepeatPad           3

#define PICT_UNKNOWN        0
#define PICT_a8r8g8b8       1
#define PICT_x8r8g8b8       2
#define PICT_x8b8g8r8       3
#define PICT_a1r5g5b5       4
#define PICT_x1r5g5b5       5
#define PICT_r5g6b5         6
#define PICT_a8             7

#define nouveau_pixmap_bo(x)    (x->bo)
#define exaGetPixmapPitch(x)    (x->pitch)

#endif /* __NV_INCLUDE_H__ */
