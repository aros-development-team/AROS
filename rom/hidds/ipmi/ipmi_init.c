/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include LC_LIBDEFS_FILE

static int IPMI_Init(struct HiddIPMIIntBase *ipmiBase)
{
    struct class_static_data    *base = &ipmiBase->hbi_csd;
    D(bug("[IPMI] %s()\n", __func__));

    HiddIPMIAB = OOP_ObtainAttrBase(IID_Hidd_IPMI);
    D(bug("[IPMI] %s: HiddIPMIAB %x @ 0x%p\n", __func__, HiddIPMIAB, &HiddIPMIAB);)

    return TRUE;
}

ADD2INITLIB(IPMI_Init, 0)
