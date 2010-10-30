#ifndef _NOUVEAU_INTERN_H
#define _NOUVEAU_INTERN_H
/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/graphics.h>
#include <hidd/i2c.h>
#include <hidd/gallium.h>

#include "nouveau/nouveau_drmif.h"
#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_notifier.h"
#include "nouveau/nouveau_grobj.h"
#include "nouveau/nouveau_pushbuf.h"

#include LC_LIBDEFS_FILE

#define CLID_Hidd_Gfx_Nouveau           "hidd.gfx.nouveau"

struct HIDDNouveauData
{
    struct nouveau_bo       *cursor;
    ULONG                   selectedcrtcid;
    ULONG                   selectedconnectorid;
};

#define CLID_Hidd_BitMap_Nouveau        "hidd.bitmap.nouveau"

struct HIDDNouveauBitMapData
{
    struct SignalSemaphore  semaphore;
    struct nouveau_bo       *bo; /* Buffer object behind bitmap. Don't make any
                                    assumptions about buffer mapping (bo->map)
                                    state. This state however can only be changed
                                    when lock is held on bitmap */

    ULONG                   height;
    ULONG                   width;
    ULONG                   pitch;
    UBYTE                   bytesperpixel;    /* In bytes */
    UBYTE                   depth;            /* In bits */
    
    ULONG                   fbid;             /* Contains ID under which bitmap 
                                                is registered as framebuffer or 
                                                0 otherwise */
};

#define CLID_Hidd_I2C_Nouveau           "hidd.i2c.nouveau"
#define IID_Hidd_I2C_Nouveau            "hidd.i2c.nouveau"

#define HiddI2CNouveauAttrBase __IHidd_I2C_Nouveau

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddI2CNouveauAttrBase;
#endif

enum
{
    aoHidd_I2C_Nouveau_Chan,                /* [I..] The nouveau_i2c_chan object */
    
    num_Hidd_I2C_Nouveau_Attrs
};

#define aHidd_I2C_Nouveau_Chan          (HiddI2CNouveauAttrBase + aoHidd_I2C_Nouveau_Chan)

#define IS_I2CNOUVEAU_ATTR(attr, idx) \
    (((idx) = (attr) - HiddI2CNouveauAttrBase) < num_Hidd_I2C_Nouveau_Attrs)

struct HIDDNouveauI2CData
{
    IPTR    i2c_chan;
};

#define CLID_Hidd_Gallium_Nouveau       "hidd.gallium.nouveau"

struct HIDDGalliumNouveauData
{
};

struct CardData
{
    /* Card controlling objects */
    ULONG                   architecture;
    struct nouveau_device   *dev;                   /* Device object acquired from libdrm */
    struct nouveau_channel  *chan;
    struct nouveau_notifier *notify0;
    
    struct nouveau_grobj    *NvImageBlit;
    struct nouveau_grobj    *NvContextSurfaces;
    struct nouveau_grobj    *NvRop;
    struct nouveau_grobj    *NvImagePattern;
    struct nouveau_grobj    *NvRectangle;
    struct nouveau_grobj    *NvMemFormat;
    struct nouveau_grobj    *Nv2D;
    
    struct nouveau_bo       *GART;                  /* Buffer in GART for upload/download of images */
    struct SignalSemaphore  gartsemaphore;
};

struct staticdata
{
    OOP_Class       *gfxclass;
    OOP_Class       *bmclass;
    OOP_Class       *i2cclass;
    OOP_Class       *galliumclass;
    
    OOP_AttrBase    pixFmtAttrBase;
    OOP_AttrBase    gfxAttrBase;
    OOP_AttrBase    syncAttrBase;
    OOP_AttrBase    bitMapAttrBase;
    OOP_AttrBase    planarAttrBase;
    OOP_AttrBase    i2cNouveauAttrBase;
    OOP_AttrBase    galliumAttrBase;
    
    OOP_MethodID    mid_CopyMemBox8;
    OOP_MethodID    mid_CopyMemBox16;
    OOP_MethodID    mid_CopyMemBox32;
    OOP_MethodID    mid_PutMem32Image8;
    OOP_MethodID    mid_PutMem32Image16;
    OOP_MethodID    mid_GetMem32Image8;
    OOP_MethodID    mid_GetMem32Image16;
    OOP_MethodID    mid_PutMemTemplate8;
    OOP_MethodID    mid_PutMemTemplate16;
    OOP_MethodID    mid_PutMemTemplate32;
    OOP_MethodID    mid_PutMemPattern8;
    OOP_MethodID    mid_PutMemPattern16;
    OOP_MethodID    mid_PutMemPattern32;
    OOP_MethodID    mid_ConvertPixels;
    OOP_MethodID    mid_GetPixFmt;

    struct CardData carddata;
    
    /* TEMP - FIXME HACK FOR GALLIUM */
    struct HIDDNouveauBitMapData * screenbitmap;
    /* TEMP - FIXME HACK FOR GALLIUM */
    
    struct SignalSemaphore multibitmapsemaphore;
    
    /* TEMP - FIXME HACK FOR PATCHRGBCONV */
    volatile BOOL rgbpatched;
    /* TEMP - FIXME HACK FOR PATCHRGBCONV */
};

LIBBASETYPE 
{
    struct Library      base;
    struct staticdata   sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib)                   ((LIBBASETYPEPTR)(lib))

#define SD(cl)                      (&BASE(cl->UserData)->sd)

#define LOCK_BITMAP                 { ObtainSemaphore(&bmdata->semaphore); }
#define UNLOCK_BITMAP               { ReleaseSemaphore(&bmdata->semaphore); }

#define LOCK_BITMAP_BM(bmdata)      { ObtainSemaphore(&(bmdata)->semaphore); }
#define UNLOCK_BITMAP_BM(bmdata)    { ReleaseSemaphore(&(bmdata)->semaphore); }

#define LOCK_MULTI_BITMAP           { ObtainSemaphore(&(SD(cl))->multibitmapsemaphore); }
#define UNLOCK_MULTI_BITMAP         { ReleaseSemaphore(&(SD(cl))->multibitmapsemaphore); }

#define UNMAP_BUFFER                { if (bmdata->bo->map) nouveau_bo_unmap(bmdata->bo); }
#define UNMAP_BUFFER_BM(bmdata)     { if ((bmdata)->bo->map) nouveau_bo_unmap((bmdata)->bo); }

#define MAP_BUFFER                  { if (!bmdata->bo->map) nouveau_bo_map(bmdata->bo, NOUVEAU_BO_RDWR); }
#define MAP_BUFFER_BM(bmdata)       { if (!(bmdata)->bo->map) nouveau_bo_map((bmdata)->bo, NOUVEAU_BO_RDWR); }

#define writel(val, addr)           (*(volatile ULONG*)(addr) = (val))
#define readl(addr)                 (*(volatile ULONG*)(addr))
#define writew(val, addr)           (*(volatile UWORD*)(addr) = (val))
#define readw(addr)                 (*(volatile UWORD*)(addr))
#define writeb(val, addr)           (*(volatile UBYTE*)(addr) = (val))
#define readb(addr)                 (*(volatile UBYTE*)(addr))

enum DMAObjects 
{
    NvNullObject        = 0x00000000,
    NvContextSurfaces   = 0x80000010, 
    NvRop               = 0x80000011, 
    NvImagePattern      = 0x80000012, 
    NvClipRectangle     = 0x80000013, 
    NvSolidLine         = 0x80000014, 
    NvImageBlit         = 0x80000015, 
    NvRectangle         = 0x80000016, 
    NvScaledImage       = 0x80000017, 
    NvMemFormat         = 0x80000018,
    Nv3D                = 0x80000019,
    NvImageFromCpu      = 0x8000001A,
    NvContextBeta1      = 0x8000001B,
    NvContextBeta4      = 0x8000001C,
    Nv2D                = 0x80000020,
    NvSW                = 0x80000021,
    NvDmaFB             = 0xD8000001,
    NvDmaTT             = 0xD8000002,
    NvDmaNotifier0      = 0xD8000003,
    NvVBlankSem         = 0xD8000004,
};

#define NV_ARCH_03  0x03
#define NV_ARCH_04  0x04
#define NV_ARCH_10  0x10
#define NV_ARCH_20  0x20
#define NV_ARCH_30  0x30
#define NV_ARCH_40  0x40
#define NV_ARCH_50  0x50

BOOL NVAccelCommonInit(struct CardData * carddata);
VOID NVAccelFree(struct CardData * carddata);
BOOL NVAccelGetCtxSurf2DFormatFromPixmap(struct HIDDNouveauBitMapData * bmdata, LONG *fmt_ret);

BOOL HIDDNouveauNV04CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    ULONG srcX, ULONG srcY, ULONG destX, ULONG destY, ULONG width, ULONG height,
    ULONG drawmode);
BOOL HIDDNouveauNV04FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, ULONG minX, ULONG minY, ULONG maxX,
    ULONG maxY, ULONG drawmode, ULONG color);
    
BOOL HIDDNouveauNV50CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    ULONG srcX, ULONG srcY, ULONG destX, ULONG destY, ULONG width, ULONG height,
    ULONG drawmode);
BOOL HIDDNouveauNV50FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, ULONG minX, ULONG minY, ULONG maxX,
    ULONG maxY, ULONG drawmode, ULONG color);
    
BOOL HiddNouveauWriteFromRAM(
    APTR src, ULONG srcPitch, HIDDT_StdPixFmt srcPixFmt,
    APTR dst, ULONG dstPitch,
    ULONG width, ULONG height,
    OOP_Class *cl, OOP_Object *o);
BOOL HiddNouveauNVAccelUploadM2MF(
    UBYTE * srcpixels, ULONG srcpitch, HIDDT_StdPixFmt srcPixFmt,
    ULONG x, ULONG y, ULONG width, ULONG height, 
    OOP_Class *cl, OOP_Object *o);

BOOL HiddNouveauReadIntoRAM(
    APTR src, ULONG srcPitch, 
    APTR dst, ULONG dstPitch, HIDDT_StdPixFmt dstPixFmt,
    ULONG width, ULONG height,
    OOP_Class *cl, OOP_Object *o);
BOOL HiddNouveauNVAccelDownloadM2MF(
    UBYTE * dstpixels, ULONG dstpitch, HIDDT_StdPixFmt dstPixFmt,
    ULONG x, ULONG y, ULONG width, ULONG height, 
    OOP_Class *cl, OOP_Object *o);   

/* Declaration of nouveau initialization function */
extern int nouveau_init(void);

#endif /* _NOUVEAU_INTERN_H */
