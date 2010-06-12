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
    { IID_Hidd_Gallium,     &LIBBASE->sd.galliumAttrBase },
    { NULL, NULL }
    };

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;
    
    LIBBASE->sd.mid_CopyMemBox8     = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox8);
    LIBBASE->sd.mid_CopyMemBox16    = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
    LIBBASE->sd.mid_CopyMemBox32    = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
    LIBBASE->sd.mid_PutMem32Image8  = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMem32Image8);
    LIBBASE->sd.mid_PutMem32Image16 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
    LIBBASE->sd.mid_GetMem32Image8  = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_GetMem32Image8);
    LIBBASE->sd.mid_GetMem32Image16 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);
	LIBBASE->sd.mid_PutMemTemplate8 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate8);
	LIBBASE->sd.mid_PutMemTemplate16= OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate16);
	LIBBASE->sd.mid_PutMemTemplate32= OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate32);
	LIBBASE->sd.mid_PutMemPattern8  = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemPattern8);
	LIBBASE->sd.mid_PutMemPattern16 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemPattern16);
	LIBBASE->sd.mid_PutMemPattern32 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemPattern32);
  
    
    InitSemaphore(&LIBBASE->sd.multibitmapsemaphore);

    return TRUE;
}

ADD2INITLIB(Novueau_Init, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 0, static struct Library *, GalliumBase);
