/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved

*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include LC_LIBDEFS_FILE

static int Power_Init(struct HiddPowerIntBase *powerBase)
{
    struct class_static_data    *base = &powerBase->hbi_csd;
    D(bug("[Power] %s()\n", __func__));

    HiddPowerAB = OOP_ObtainAttrBase(IID_Hidd_Power);
    D(bug("[Power] %s: HiddPowerAB %x @ 0x%p\n", __func__, HiddPowerAB, &HiddPowerAB);)

    return TRUE;
}

ADD2INITLIB(Power_Init, 0)
