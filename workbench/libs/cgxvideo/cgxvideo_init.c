/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <hidd/gfx.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

static int cgxv_init(struct IntCGXVBase *CGXVideoBase)
{
    CGXVideoBase->attrbases[0].interfaceID = IID_Hidd_BitMap;
    CGXVideoBase->attrbases[0].attrBase = &HiddBitMapAttrBase;

    return OOP_ObtainAttrBases(CGXVideoBase->attrbases);
}

static int cgxv_expunge(struct IntCGXVBase *CGXVideoBase)
{
    OOP_ReleaseAttrBases(CGXVideoBase->attrbases);

    return TRUE;
}

ADD2INITLIB(cgxv_init, 0);
ADD2EXPUNGELIB(cgxv_expunge, 0);
