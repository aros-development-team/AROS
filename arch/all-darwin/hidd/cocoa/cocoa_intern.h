#ifndef COCOA_INTERN_H
#define COCOA_INTERN_H

#include <hidd/gfx.h>
#include <oop/oop.h>
#include <exec/libraries.h>

struct HostInterface;

struct CocoaGfx_staticdata {
    OOP_Class         *gfxclass;
    OOP_Class         *bmclass;

    OOP_AttrBase       hiddBitMapAttrBase;
    OOP_AttrBase       hiddGfxAttrBase;
    OOP_AttrBase       hiddSyncAttrBase;
    OOP_AttrBase       hiddPixFmtAttrBase;

    struct HostInterface *iface;
    void              *fb_base;
    int                fb_width;
    int                fb_height;
    int                fb_pitch;
};

struct CocoaBMData {
    void   *pixels;
    int     width;
    int     height;
    int     pitch;
    int     bpp;
    BOOL    is_fb;
};

/* The static data is stored in class->UserData */
#define CSD(cl) ((struct CocoaGfx_staticdata *)cl->UserData)

/* AttrBase variables - defined in startup.c */
extern OOP_AttrBase __IHidd;
extern OOP_AttrBase __IHidd_BitMap;
extern OOP_AttrBase __IHidd_Gfx;
extern OOP_AttrBase __IHidd_Sync;
extern OOP_AttrBase __IHidd_PixFmt;
extern OOP_AttrBase __IHidd_ChunkyBM;

/* Attribute base shortcuts used by IS_GFX_ATTR etc. */
#define HiddBitMapAttrBase  __IHidd_BitMap
#define HiddGfxAttrBase     __IHidd_Gfx
#define HiddSyncAttrBase    __IHidd_Sync
#define HiddPixFmtAttrBase  __IHidd_PixFmt
#define HiddChunkyBMAttrBase __IHidd_ChunkyBM

#endif /* COCOA_INTERN_H */
