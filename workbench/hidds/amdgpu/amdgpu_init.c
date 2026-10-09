/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: AMDGPU Hidd initialisation code
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "amdgpu_intern.h"

#include LC_LIBDEFS_FILE

static OOP_AttrBase HiddPixFmtAttrBase; // = 0;

static struct OOP_ABDescr abd[] =
{
    { IID_Hidd_PixFmt,  &HiddPixFmtAttrBase },
    { NULL,             NULL                }
};

static int Amdgpu_Init(LIBBASETYPEPTR LIBBASE)
{
    struct Amdgpu_staticdata *xsd = &LIBBASE->vsd;

    D(bug("[amdgpu.hidd] %s: code @ 0x%p\n", __func__, Amdgpu_KMS_Init);)

    xsd->AmdgpuCyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library",0);
    if (xsd->AmdgpuCyberGfxBase == NULL)
        goto failure;

    if (!OOP_ObtainAttrBases(abd))
        goto failure;

    xsd->basebm = OOP_FindClass(CLID_Hidd_BitMap);
    xsd->basegallium = OOP_FindClass(CLID_Hidd_Gallium);

    xsd->hiddGalliumAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);
    if (xsd->hiddGalliumAB == 0)
        goto failure;

    /*
     * The amdgpu driver finds the card, brings it up and owns it from
     * here on; the hidd talks to it as a DRM client.
     */
    if (!Amdgpu_KMS_Init(&xsd->kms))
    {
        /* with its tasks up the driver cannot be unloaded; the class just declines */
        if (xsd->kms.started)
            return TRUE;
        goto failure;
    }

    D(bug("[amdgpu.hidd] %s: Suitable adaptor found\n", __func__);)
    return TRUE;

failure:
    D(bug("[amdgpu.hidd] %s: No suitable adaptors found\n", __func__);)

    if (xsd->AmdgpuCyberGfxBase)
        CloseLibrary(xsd->AmdgpuCyberGfxBase);

    if (xsd->hiddGalliumAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    OOP_ReleaseAttrBases(abd);

    return FALSE;
}

ADD2INITLIB(Amdgpu_Init, 0)

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
ADD2LIBS((STRPTR)"pci.hidd", 0, static struct Library *, PciHiddBase);
