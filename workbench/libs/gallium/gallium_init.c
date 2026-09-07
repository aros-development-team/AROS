/*
    Copyright (C) 2010-2019, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>

#include <dos/var.h>
#include <hidd/gfx.h>

#include <string.h>

#include LC_LIBDEFS_FILE
#include "gallium_intern.h"

CONST_STRPTR softpipe_str = "softpipe";

static int Init(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->gfxAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gfx);
    if (!LIBBASE->gfxAttrBase)
        return FALSE;

    LIBBASE->bmAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap);
    if (!LIBBASE->bmAttrBase)
        return FALSE;

    LIBBASE->galliumAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);
    if (!LIBBASE->galliumAttrBase)
        return FALSE;

    LIBBASE->basegallium = OOP_FindClass(CLID_Hidd_Gallium);
    LIBBASE->fallback = (char *)softpipe_str;
    LIBBASE->fallbackmodule = NULL;

    /* Which software rasteriser to use when the display has no gallium
       driver of its own. (llvmpipe, softpipe) */
    {
        char buf[64];

        if (GetVar("SYS/Gallium.default", buf, sizeof(buf),
                   GVF_GLOBAL_ONLY | LV_VAR) > 0)
        {
            char *sel = AllocVec(strlen(buf) + 1, MEMF_PUBLIC);

            if (sel)
            {
                strcpy(sel, buf);
                LIBBASE->fallback = sel;
            }
        }
    }

    /* cache method id's that we use ..  */
    LIBBASE->galliumMId_UpdateRect = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UpdateRect);
    LIBBASE->galliumMId_DisplayResource = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplayResource);
    LIBBASE->galliumMId_DisplayResourceRP = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplayResourceRP);

    return TRUE;
}

static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    
    if (LIBBASE->galliumAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    if (LIBBASE->bmAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_BitMap);

    if (LIBBASE->gfxAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gfx);

    if (LIBBASE->fallbackmodule)
        CloseLibrary(LIBBASE->fallbackmodule);

    if ((CONST_STRPTR)LIBBASE->fallback != softpipe_str)
        FreeVec(LIBBASE->fallback);

    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 0, static struct Library *, GalliumHiddBase);
ADD2LIBS((STRPTR)"dos.library", 0, struct DosLibrary *, DOSBase);
