/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

static ULONG Novueau_Init(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = 
    {
    { IID_Hidd_BitMap,      &LIBBASE->sd.bitMapAttrBase },
    { IID_Hidd_PixFmt,	    &LIBBASE->sd.pixFmtAttrBase },
    { IID_Hidd_Sync,        &LIBBASE->sd.syncAttrBase },
    { IID_Hidd_Gfx,         &LIBBASE->sd.gfxAttrBase },
    { IID_Hidd_PlanarBM,    &LIBBASE->sd.planarAttrBase },
    { IID_Hidd_I2C_Nouveau, &LIBBASE->sd.i2cNouveauAttrBase },
    { IID_Hidd_GalliumBaseDriver,   &LIBBASE->sd.galliumAttrBase },
    { NULL, NULL }
    };

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;
    
    InitSemaphore(&LIBBASE->sd.multibitmapsemaphore);

    return TRUE;
}

ADD2INITLIB(Novueau_Init, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 0, static struct Library *, GalliumBase);
