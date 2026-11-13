/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: HD Audio controller hidd, library initialization
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/libraries.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/hda.h>

#include "hda_intern.h"

#include LC_LIBDEFS_FILE

static int HDA_InitLib(LIBBASETYPEPTR LIBBASE)
{
    struct hda_staticdata *hsd = &LIBBASE->hsd;

    D(bug("[HDA] %s()\n", __func__));

    hsd->utilityBase = OpenLibrary("utility.library", 36);
    if (!hsd->utilityBase)
        return FALSE;

    hsd->hdaAttrBase = OOP_ObtainAttrBase(IID_Hidd_HDA);

    if (!hsd->hdaAttrBase)
        return FALSE;

    return TRUE;
}

static int HDA_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    struct hda_staticdata *hsd = &LIBBASE->hsd;

    D(bug("[HDA] %s()\n", __func__));

    if (hsd->hdaAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_HDA);

    if (hsd->utilityBase)
        CloseLibrary(hsd->utilityBase);

    return TRUE;
}

ADD2INITLIB(HDA_InitLib, 0)
ADD2EXPUNGELIB(HDA_ExpungeLib, 0)
