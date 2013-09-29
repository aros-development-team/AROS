#ifndef _NOUVEAU_INTERN_H
#define _NOUVEAU_INTERN_H
/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
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

#include LC_LIBDEFS_FILE

#define CLID_Hidd_Gfx_Nouveau           "hidd.gfx.nouveau"
#define IID_Hidd_Gfx_Nouveau            "hidd.gfx.nouveau"

#define HiddGfxNouveauAttrBase          __IHidd_Gfx_Nouveau

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddGfxNouveauAttrBase;
#endif

enum
{
    aoHidd_Gfx_Nouveau_VRAMSize,        /* [G..] The amount of total VRAM in bytes */
    aoHidd_Gfx_Nouveau_GARTSize,        /* [G..] The amount of total GART in bytes */
    aoHidd_Gfx_Nouveau_VRAMFree,        /* [G..] The amount of free VRAM in bytes */
    aoHidd_Gfx_Nouveau_GARTFree,        /* [G..] The amount of free GART in bytes */
    
    num_Hidd_Gfx_Nouveau_Attrs
};

#define aHidd_Gfx_Nouveau_VRAMSize      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_VRAMSize)
#define aHidd_Gfx_Nouveau_GARTSize      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_GARTSize)
#define aHidd_Gfx_Nouveau_VRAMFree      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_VRAMFree)
#define aHidd_Gfx_Nouveau_GARTFree      (HiddGfxNouveauAttrBase + aoHidd_Gfx_Nouveau_GARTFree)

#define IS_GFXNOUVEAU_ATTR(attr, idx) \
    (((idx) = (attr) - HiddGfxNouveauAttrBase) < num_Hidd_Gfx_Nouveau_Attrs)

struct HIDDNouveauData
{
    struct nouveau_bo   *cursor;
    ULONG               selectedcrtcid;
    APTR                selectedmode;
    APTR                selectedconnector;
    OOP_Object          *compositor;
};

#define CLID_Hidd_BitMap_Nouveau        "hidd.bitmap.nouveau"
#define IID_Hidd_BitMap_Nouveau         "hidd.bitmap.nouveau"

#define HiddBitMapNouveauAttrBase __IHidd_BitMap_Nouveau

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddBitMapNouveauAttrBase;
#endif

enum
{
    aoHidd_BitMap_Nouveau_CompositorHidd,       /* [I..] The compositor object that will be used by bitmap */
    
    num_Hidd_BitMap_Nouveau_Attrs
};

#define aHidd_BitMap_Nouveau_CompositorHidd    (HiddBitMapNouveauAttrBase + aoHidd_BitMap_Nouveau_CompositorHidd)

#define IS_BITMAPNOUVEAU_ATTR(attr, idx) \
    (((idx) = (attr) - HiddBitMapNouveauAttrBase) < num_Hidd_BitMap_Nouveau_Attrs)

struct HIDDNouveauBitMapData
{
    struct SignalSemaphore  semaphore;
    struct nouveau_bo       *bo; /* Buffer object behind bitmap. Don't make any
                                    assumptions about buffer mapping (bo->map)
                                    state. This state however can only be changed
                                    when lock is held on bitmap */

    ULONG   height;         /* Height of bitmap in pixels */
    ULONG   width;          /* Width of bitmap in pixels */
    ULONG   pitch;          /* Width of single data row in bytes */
    UBYTE   bytesperpixel;  /* In bytes, how many bytes to store a pixel */
    UBYTE   depth;          /* In bits, how many bits used to represt the color */
    BOOL    displayable;    /* Can bitmap be displayed on screen */
    
    /* Information connected with display */
    OOP_Object  *compositor;   /* Compositor object used by bitmap */
    LONG        xoffset;        /* Offset to bitmap point that is displayed as (0,0) on screen */
    LONG        yoffset;        /* Offset to bitmap point that is displayed as (0,0) on screen */
    ULONG       fbid;           /* Contains ID under which bitmap 
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
    BOOL                    IsPCIE;
    struct nouveau_device   *dev;                   /* Device object acquired from libdrm */
    struct nouveau_channel  *chan;

    struct nouveau_notifier *notify0;
    struct nouveau_notifier *vblank_sem;
    
    struct nouveau_grobj    *NvImageBlit;
    struct nouveau_grobj    *NvContextSurfaces;
    struct nouveau_grobj    *NvRop;
    struct nouveau_grobj    *NvImagePattern;
    struct nouveau_grobj    *NvRectangle;
    struct nouveau_grobj    *NvMemFormat;
    struct nouveau_grobj    *Nv2D;
    struct nouveau_grobj    *Nv3D;
    struct nouveau_grobj    *NvSW;
    struct nouveau_bo       *shader_mem;
    struct nouveau_bo       *tesla_scratch;
    
    struct nouveau_bo       *GART;                  /* Buffer in GART for upload/download of images */
    struct SignalSemaphore  gartsemaphore;
};

struct staticdata
{
    OOP_Class       *gfxclass;
    OOP_Class       *bmclass;
    OOP_Class       *i2cclass;
    OOP_Class       *galliumclass;
    OOP_Class       *compositorclass;
    
    OOP_AttrBase    pixFmtAttrBase;
    OOP_AttrBase    gfxAttrBase;
    OOP_AttrBase    gfxNouveauAttrBase;
    OOP_AttrBase    syncAttrBase;
    OOP_AttrBase    bitMapAttrBase;
    OOP_AttrBase    planarAttrBase;
    OOP_AttrBase    i2cNouveauAttrBase;
    OOP_AttrBase    galliumAttrBase;
    OOP_AttrBase    gcAttrBase;
    OOP_AttrBase    compositorAttrBase;
    OOP_AttrBase    bitMapNouveauAttrBase;
    
    OOP_MethodID    mid_CopyMemBox16;
    OOP_MethodID    mid_CopyMemBox32;
    OOP_MethodID    mid_PutMem32Image16;
    OOP_MethodID    mid_GetMem32Image16;
    OOP_MethodID    mid_PutMemTemplate16;
    OOP_MethodID    mid_PutMemTemplate32;
    OOP_MethodID    mid_PutMemPattern16;
    OOP_MethodID    mid_PutMemPattern32;
    OOP_MethodID    mid_ConvertPixels;
    OOP_MethodID    mid_GetPixFmt;
    
    OOP_MethodID    mid_BitMapPositionChanged;
    OOP_MethodID    mid_BitMapRectChanged;
    OOP_MethodID    mid_ValidateBitMapPositionChange;

    struct CardData carddata;
    
    struct SignalSemaphore multibitmapsemaphore;
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

#define IS_NOUVEAU_BM_CLASS(x)      ((x) == SD(cl)->bmclass)

#define writel(val, addr)           (*(volatile ULONG*)(addr) = (val))
#define readl(addr)                 (*(volatile ULONG*)(addr))
#define writew(val, addr)           (*(volatile UWORD*)(addr) = (val))
#define readw(addr)                 (*(volatile UWORD*)(addr))

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
#define NV_ARCH_C0  0xC0

#define BLENDOP_SOLID           1
#define BLENDOP_ALPHA_PREMULT   3
#define BLENDOP_ALPHA           13

/* nv_accel_common.c */
BOOL HIDDNouveauAccelCommonInit(struct CardData * carddata);
VOID HIDDNouveauAccelFree(struct CardData * carddata);

BOOL NVAccelGetCtxSurf2DFormatFromPixmap(struct HIDDNouveauBitMapData * bmdata, LONG *fmt_ret);

/* nv04_exa.c */
VOID HIDDNouveauNV04SetPattern(struct CardData * carddata, LONG clr0, LONG clr1,
		  LONG pat0, LONG pat1);
BOOL HIDDNouveauNV04FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, LONG minX, LONG minY, LONG maxX,
    LONG maxY, ULONG drawmode, ULONG color);
BOOL HIDDNouveauNV04CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG drawmode);

/* nv10_exa.c */
BOOL HIDDNouveauNV103DCopyBox(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG blendop);

/* nv30_exa.c */
BOOL HIDDNouveauNV303DCopyBox(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG blendop);

/* nv40_exa.c */
BOOL HIDDNouveauNV403DCopyBox(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG blendop);

/* nv50_exa.c */
VOID HIDDNouveauNV50SetPattern(struct CardData * carddata, LONG col0, 
    LONG col1, LONG pat0, LONG pat1);
BOOL HIDDNouveauNV50FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, LONG minX, LONG minY, LONG maxX,
    LONG maxY, ULONG drawmode, ULONG color);
BOOL HIDDNouveauNV50CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG drawmode);

/* nvc0_exa.c */
VOID HIDDNouveauNVC0SetPattern(struct CardData * carddata, LONG clr0, LONG clr1,
		  LONG pat0, LONG pat1);
BOOL HIDDNouveauNVC0FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, LONG minX, LONG minY, LONG maxX,
    LONG maxY, ULONG drawmode, ULONG color);
BOOL HIDDNouveauNVC0CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG drawmode);

/* nouveau_accel.c */
BOOL HiddNouveauWriteFromRAM(
    APTR src, ULONG srcPitch, HIDDT_StdPixFmt srcPixFmt,
    APTR dst, ULONG dstPitch,
    ULONG width, ULONG height,
    OOP_Class *cl, OOP_Object *o);
BOOL HiddNouveauReadIntoRAM(
    APTR src, ULONG srcPitch, 
    APTR dst, ULONG dstPitch, HIDDT_StdPixFmt dstPixFmt,
    ULONG width, ULONG height,
    OOP_Class *cl, OOP_Object *o);
BOOL HiddNouveauAccelARGBUpload3D(
    UBYTE * srcpixels, ULONG srcpitch,
    LONG x, LONG y, LONG width, LONG height, 
    OOP_Class *cl, OOP_Object *o);
BOOL HiddNouveauAccelAPENUpload3D(
    UBYTE * srcalpha, BOOL srcinvertalpha, ULONG srcpitch, ULONG srcpenrgb,
    LONG x, LONG y, LONG width, LONG height, 
    OOP_Class *cl, OOP_Object *o);
VOID HIDDNouveauBitMapPutAlphaImage32(struct HIDDNouveauBitMapData * bmdata,
    APTR srcbuff, ULONG srcpitch, LONG destX, LONG destY, LONG width, LONG height);
VOID HIDDNouveauBitMapPutAlphaImage16(struct HIDDNouveauBitMapData * bmdata,
    APTR srcbuff, ULONG srcpitch, LONG destX, LONG destY, LONG width, LONG height);
VOID HIDDNouveauBitMapPutAlphaTemplate32(struct HIDDNouveauBitMapData * bmdata,
    OOP_Object * gc, OOP_Object * bm, BOOL invertalpha,
    UBYTE * srcalpha, ULONG srcpitch, LONG destX, LONG destY, LONG width, LONG height);
VOID HIDDNouveauBitMapPutAlphaTemplate16(struct HIDDNouveauBitMapData * bmdata,
    OOP_Object * gc, OOP_Object * bm, BOOL invertalpha,
    UBYTE * srcalpha, ULONG srcpitch, LONG destX, LONG destY, LONG width, LONG height);
VOID HIDDNouveauBitMapDrawSolidLine(struct HIDDNouveauBitMapData * bmdata,
    OOP_Object * gc, LONG destX1, LONG destY1, LONG destX2, LONG destY2);

/* nouveau_exa.c */
BOOL HiddNouveauNVAccelUploadM2MF(
    UBYTE * srcpixels, ULONG srcpitch, HIDDT_StdPixFmt srcPixFmt,
    LONG x, LONG y, LONG width, LONG height, 
    OOP_Class *cl, OOP_Object *o);
BOOL HiddNouveauNVAccelDownloadM2MF(
    UBYTE * dstpixels, ULONG dstpitch, HIDDT_StdPixFmt dstPixFmt,
    LONG x, LONG y, LONG width, LONG height, 
    OOP_Class *cl, OOP_Object *o);

VOID HIDDNouveauShowCursor(OOP_Object * gfx, BOOL visible);
BOOL HIDDNouveauSwitchToVideoMode(OOP_Object * bm);
VOID HIDDNouveauSetOffsets(OOP_Object * bm, LONG newxoffset, LONG newyoffset);

/* Declaration of nouveau initialization function */
extern int nouveau_init(void);

/* Commom memory allocation */
APTR HIDDNouveauAlloc(ULONG size);
VOID HIDDNouveauFree(APTR memory);

#endif /* _NOUVEAU_INTERN_H */
