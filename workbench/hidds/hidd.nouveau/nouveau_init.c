/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
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
    OOP_NewObject(NULL, CLID_Hidd_Gfx_Nouveau, NULL);
    
    /* TODO: NewBitmap creation of screen bitmap 1024x768 */
    /* TODO: ShowViewPorts - display of bitmap */
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

    LIBBASE->sd.mid_BitMapPositionChanged   =
        OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapPositionChanged);
    LIBBASE->sd.mid_BitMapRectChanged       = 
        OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapRectChanged);
    LIBBASE->sd.mid_ValidateBitMapPositionChange =
        OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_ValidateBitMapPositionChange);

  
    
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

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
