/*
    Copyright � 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"
#include "drm_aros_config.h"

#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#if defined(HOSTED_BUILD)
static VOID Nouveau_HOSTED_BUILD_Init(LIBBASETYPEPTR LIBBASE)
{
    struct nouveau_device * dev = NULL;
    struct nouveau_device_priv * nvdev = NULL;
    struct CardData * carddata = &LIBBASE->sd.carddata;

    nouveau_init();

    nouveau_device_open(&dev, "");
    nvdev = nouveau_device(dev);

    carddata->dev = dev;
    
    /* Check chipset architecture */
    switch (carddata->dev->chipset & 0xf0) 
    {
    case 0x00:
        carddata->architecture = NV_ARCH_04;
        break;
    case 0x10:
        carddata->architecture = NV_ARCH_10;
        break;
    case 0x20:
        carddata->architecture = NV_ARCH_20;
        break;
    case 0x30:
        carddata->architecture = NV_ARCH_30;
        break;
    case 0x40:
    case 0x60:
        carddata->architecture = NV_ARCH_40;
        break;
    case 0x50:
    case 0x80:
    case 0x90:
    case 0xa0:
        carddata->architecture = NV_ARCH_50;
        break;
    default:
        break;
    }

    /* Allocate dma channel */
    nouveau_channel_alloc(carddata->dev, NvDmaFB, NvDmaTT, &carddata->chan);

    /* Initialize acceleration objects */
    NVAccelCommonInit(carddata);
    
    /* Partial initialization of screenbitmap - 1024x768x24 */
    LIBBASE->sd.screenbitmap = AllocVec(sizeof(struct HIDDNouveauBitMapData), MEMF_ANY | MEMF_CLEAR);
    LIBBASE->sd.screenbitmap->width = 1024;
    LIBBASE->sd.screenbitmap->height = 768;
    LIBBASE->sd.screenbitmap->depth = 24;
    if (LIBBASE->sd.screenbitmap->depth <= 8)
        LIBBASE->sd.screenbitmap->bytesperpixel = 1;
    else if (LIBBASE->sd.screenbitmap->depth <= 16)
        LIBBASE->sd.screenbitmap->bytesperpixel = 2;
    else
        LIBBASE->sd.screenbitmap->bytesperpixel = 4;
    LIBBASE->sd.screenbitmap->pitch = 1024 * 4;
    LIBBASE->sd.screenbitmap->fbid = 0; /* Default value */
    InitSemaphore(&LIBBASE->sd.screenbitmap->semaphore);

    nouveau_bo_new(carddata->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 0, 
            LIBBASE->sd.screenbitmap->pitch * LIBBASE->sd.screenbitmap->height,
            &LIBBASE->sd.screenbitmap->bo);

    nouveau_bo_map(LIBBASE->sd.screenbitmap->bo, NOUVEAU_BO_RDWR);
}
#endif

static ULONG Novueau_Init(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = 
    {
    { IID_Hidd_BitMap,          &LIBBASE->sd.bitMapAttrBase },
    { IID_Hidd_PixFmt,	        &LIBBASE->sd.pixFmtAttrBase },
    { IID_Hidd_Sync,            &LIBBASE->sd.syncAttrBase },
    { IID_Hidd_Gfx,             &LIBBASE->sd.gfxAttrBase },
    { IID_Hidd_PlanarBM,        &LIBBASE->sd.planarAttrBase },
    { IID_Hidd_I2C_Nouveau,     &LIBBASE->sd.i2cNouveauAttrBase },
    { IID_Hidd_Gallium,         &LIBBASE->sd.galliumAttrBase },
    { IID_Hidd_GC,              &LIBBASE->sd.gcAttrBase },
    { IID_Hidd_Compositing,     &LIBBASE->sd.compositingAttrBase },
    { IID_Hidd_Bitmap_Nouveau,  &LIBBASE->sd.bitMapNouveauAttrBase },
    { NULL, NULL }
    };

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;
    
    LIBBASE->sd.mid_CopyMemBox8     = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox8);
    LIBBASE->sd.mid_CopyMemBox16    = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
    LIBBASE->sd.mid_CopyMemBox32    = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
    LIBBASE->sd.mid_PutMem32Image8  = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image8);
    LIBBASE->sd.mid_PutMem32Image16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
    LIBBASE->sd.mid_GetMem32Image8  = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image8);
    LIBBASE->sd.mid_GetMem32Image16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);
    LIBBASE->sd.mid_PutMemTemplate8 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate8);
    LIBBASE->sd.mid_PutMemTemplate16= OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate16);
    LIBBASE->sd.mid_PutMemTemplate32= OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate32);
    LIBBASE->sd.mid_PutMemPattern8  = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern8);
    LIBBASE->sd.mid_PutMemPattern16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern16);
    LIBBASE->sd.mid_PutMemPattern32 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern32);
    LIBBASE->sd.mid_ConvertPixels   = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_ConvertPixels);
    LIBBASE->sd.mid_GetPixFmt       = OOP_GetMethodID((STRPTR)IID_Hidd_Gfx, moHidd_Gfx_GetPixFmt);

    LIBBASE->sd.mid_BitMapPositionChanged   = OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapPositionChanged);
    LIBBASE->sd.mid_BitMapRectChanged       = OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapRectChanged);

  
    
    InitSemaphore(&LIBBASE->sd.multibitmapsemaphore);

#if defined(HOSTED_BUILD)
    /* This is used only for HOSTED_BUILD initialization */
    Nouveau_HOSTED_BUILD_Init(LIBBASE);
#endif

    /* TEMP - FIXME HACK FOR PATCHRGBCONV */
    LIBBASE->sd.rgbpatched = FALSE;
    /* TEMP - FIXME HACK FOR PATCHRGBCONV */

    return TRUE;
}

ADD2INITLIB(Novueau_Init, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 0, static struct Library *, GalliumHiddBase);
