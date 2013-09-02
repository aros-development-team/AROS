/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"
#include "drm_aros_config.h"

#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

/* GLOBALS */
APTR NouveauMemPool;

/* This pointer is necessary to limit the number of changes function signatures
   of xf86-video-nouveau codes. Without, carddata needs to be passed to each
   function, since in original codes the data it represents is taken from global
   array */
struct CardData * globalcarddataptr;
/* GLOBALS END */

static ULONG Nouveau_Init(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = 
    {
    { IID_Hidd_BitMap,          &LIBBASE->sd.bitMapAttrBase },
    { IID_Hidd_PixFmt,	        &LIBBASE->sd.pixFmtAttrBase },
    { IID_Hidd_Sync,            &LIBBASE->sd.syncAttrBase },
    { IID_Hidd_Gfx,             &LIBBASE->sd.gfxAttrBase },
    { IID_Hidd_Gfx_Nouveau,     &LIBBASE->sd.gfxNouveauAttrBase },
    { IID_Hidd_PlanarBM,        &LIBBASE->sd.planarAttrBase },
    { IID_Hidd_I2C_Nouveau,     &LIBBASE->sd.i2cNouveauAttrBase },
    { IID_Hidd_Gallium,         &LIBBASE->sd.galliumAttrBase },
    { IID_Hidd_GC,              &LIBBASE->sd.gcAttrBase },
    { IID_Hidd_Compositing,     &LIBBASE->sd.compositingAttrBase },
    { IID_Hidd_BitMap_Nouveau,  &LIBBASE->sd.bitMapNouveauAttrBase },
    { NULL, NULL }
    };

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;
    
    LIBBASE->sd.mid_CopyMemBox16    = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
    LIBBASE->sd.mid_CopyMemBox32    = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
    LIBBASE->sd.mid_PutMem32Image16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
    LIBBASE->sd.mid_GetMem32Image16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);
    LIBBASE->sd.mid_PutMemTemplate16= OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate16);
    LIBBASE->sd.mid_PutMemTemplate32= OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate32);
    LIBBASE->sd.mid_PutMemPattern16 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern16);
    LIBBASE->sd.mid_PutMemPattern32 = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern32);
    LIBBASE->sd.mid_ConvertPixels   = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, moHidd_BitMap_ConvertPixels);
    LIBBASE->sd.mid_GetPixFmt       = OOP_GetMethodID((STRPTR)IID_Hidd_Gfx, moHidd_Gfx_GetPixFmt);

    LIBBASE->sd.mid_BitMapPositionChanged   =
        OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapPositionChanged);
    LIBBASE->sd.mid_BitMapRectChanged       = 
        OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapRectChanged);
    LIBBASE->sd.mid_ValidateBitMapPositionChange =
        OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_ValidateBitMapPositionChange);

  
    
    InitSemaphore(&LIBBASE->sd.multibitmapsemaphore);

    /* TEMP - FIXME HACK FOR PATCHRGBCONV */
    LIBBASE->sd.rgbpatched = FALSE;
    /* TEMP - FIXME HACK FOR PATCHRGBCONV */

    NouveauMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 32 * 1024, 16 * 1024);
    
    globalcarddataptr = &LIBBASE->sd.carddata;

    return TRUE;
}

static VOID Nouveau_Exit(LIBBASETYPEPTR LIBBASE)
{
    if (NouveauMemPool)
    {
        DeletePool(NouveauMemPool);
        NouveauMemPool = NULL;
    }
}

APTR HIDDNouveauAlloc(ULONG size)
{
    return AllocVecPooled(NouveauMemPool, size);
}

VOID HIDDNouveauFree(APTR memory)
{
    FreeVecPooled(NouveauMemPool, memory);
}

ADD2INITLIB(Nouveau_Init, 0);
ADD2EXPUNGELIB(Nouveau_Exit, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
ADD2LIBS((STRPTR)"pci.hidd", 0, static struct Library *, PciHiddBase);
