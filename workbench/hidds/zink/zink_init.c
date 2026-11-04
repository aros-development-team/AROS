/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <hidd/gallium.h>
#include <hidd/gfx.h>
#include <hidd/vulkan_hidd.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "zink_intern.h"

static int HiddZink_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.UtilityBase)
        CloseLibrary(LIBBASE->sd.UtilityBase);

    if (LIBBASE->sd.CyberGfxBase)
        CloseLibrary(LIBBASE->sd.CyberGfxBase);

    if (LIBBASE->sd.hiddGalliumAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);
    if (LIBBASE->sd.hiddBitMapAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_BitMap);
    if (LIBBASE->sd.hiddPixFmtAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_PixFmt);

    return TRUE;
}

static int HiddZink_InitLib(LIBBASETYPEPTR LIBBASE)
{
    if ((LIBBASE->sd.UtilityBase = OpenLibrary((STRPTR)"utility.library",0)))
    {
        if ((LIBBASE->sd.CyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library",0)))
        {
            if ((LIBBASE->sd.hiddGalliumAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium)))
            {
                /* The Vulkan interface is served by the vulkan.hidd class
                 * the object passed to New() belongs to; only its method
                 * base is needed here. */
                LIBBASE->sd.vulkanMB = OOP_GetMethodID((STRPTR)IID_Hidd_Vulkan, 0);
                /* scan-out bitmaps (ResourceFromBitMap): without these
                 * bases the method declines and the blit path is used */
                LIBBASE->sd.hiddBitMapAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap);
                LIBBASE->sd.hiddPixFmtAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_PixFmt);
                LIBBASE->sd.bitmapMB = OOP_GetMethodID((STRPTR)IID_Hidd_BitMap, 0);
                return TRUE;
            }
            CloseLibrary(LIBBASE->sd.CyberGfxBase);
        }
        CloseLibrary(LIBBASE->sd.UtilityBase);
    }
    return FALSE;
}

ADD2INITLIB(HiddZink_InitLib, 0)
ADD2EXPUNGELIB(HiddZink_ExpungeLib, 0)

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
