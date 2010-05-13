#ifndef _NOUVEAU_INTERN_H
#define _NOUVEAU_INTERN_H
/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/graphics.h>
#include <hidd/i2c.h>

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
    ULONG                   architecture;
    struct nouveau_device   *dev;        /* Device object acquired from libdrm */
    struct nouveau_channel  *chan;
    struct nouveau_notifier *notify0;
    
    struct nouveau_grobj    *NvImageBlit;
    struct nouveau_grobj    *NvContextSurfaces;
    struct nouveau_grobj    *NvRop;
    struct nouveau_grobj    *NvImagePattern;
    
    struct nouveau_bo       *cursor;
    ULONG                   selectedcrtcid;
};

#define CLID_Hidd_BitMap_Nouveau        "hidd.bitmap.nouveau"
#define CLID_Hidd_BitMap_NouveauOff     "hidd.bitmap.nouveauoff"
#define CLID_Hidd_BitMap_NouveauPlanar  "hidd.bitmap.nouveauplanar"

struct HIDDNouveauBitMapData
{
    struct nouveau_bo * bo; /* Buffer object behind bitmap */

    ULONG height;
    ULONG width;
    ULONG pitch;
    UBYTE bytesperpixel;    /* In bytes */
    UBYTE depth;            /* In bits */
    
    ULONG fbid;             /* Contains ID under which bitmap is registered
                               as framebuffer or 0 otherwise */
};

struct planarbm_data
{
    UBYTE   **planes;
    ULONG   planebuf_size;
    ULONG   bytesperrow;
    ULONG   rows;
    UBYTE   depth;
    BOOL    planes_alloced;
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

struct staticdata
{
    OOP_Class       *gfxclass;
    OOP_Class       *bmclass;
    OOP_Class       *planarbmclass;
    OOP_Class       *i2cclass;
    
    OOP_AttrBase    pixFmtAttrBase;
    OOP_AttrBase    gfxAttrBase;
    OOP_AttrBase    syncAttrBase;
    OOP_AttrBase    bitMapAttrBase;
    OOP_AttrBase    planarAttrBase;
    OOP_AttrBase    i2cNouveauAttrBase;
};

LIBBASETYPE 
{
    struct Library      base;
    struct staticdata   sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#define writel(val, addr)               (*(volatile ULONG*)(addr) = (val))
#define readl(addr)                     (*(volatile ULONG*)(addr))

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

BOOL NVAccelCommonInit(struct HIDDNouveauData * gfxdata);
VOID NVAccelFree(struct HIDDNouveauData * gfxdata);
BOOL NVAccelGetCtxSurf2DFormatFromPixmap(struct HIDDNouveauBitMapData * bmdata, LONG *fmt_ret);

#endif /* _NOUVEAU_INTERN_H */
