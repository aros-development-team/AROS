/*
    Copyright © 2019, The AROS Development Team. All rights reserved
    $Id$

    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include LC_LIBDEFS_FILE

static int Bus_Init(struct HiddBusIntBase *busBase)
{
    struct class_static_data    *base = &busBase->hbi_csd;
    D(bug("[Bus] %s()\n", __func__));

    HiddBusAB = OOP_ObtainAttrBase(IID_Hidd_Bus);
    D(bug("[Bus] %s: HiddBusAB %x @ 0x%p\n", __func__, HiddBusAB, &HiddBusAB);)

    return TRUE;
}

ADD2INITLIB(Bus_Init, 0)
