/*
    Copyright © 2013, The AROS Development Team. All rights reserved
    $Id$

    Desc: A600/A1200/A4000 ATA HIDD
    Lang: English
*/

#include <aros/symbolsets.h>
#include <hidd/ata.h>
#include <hidd/hidd.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "bus_class.h"

static int gayleata_init(struct ataBase *base)
{
    base->cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!base->cs_UtilityBase)
        return FALSE;

    HiddAttrBase = OOP_ObtainAttrBase(IID_Hidd);
    HiddATABusAB = OOP_ObtainAttrBase(IID_Hidd_ATABus);

    if (!HiddAttrBase || !HiddATABusAB)
        return FALSE;

    return TRUE;
}

static int gayleata_expunge(struct ataBase *base)
{
    OOP_ReleaseAttrBase(HiddAttrBase);
    OOP_ReleaseAttrBase(HiddATABusAB);
    CloseLibrary(base->cs_UtilityBase);

    return TRUE;
}

ADD2INITLIB(gayleata_init, 0)
ADD2EXPUNGELIB(gayleata_expunge, 0)
