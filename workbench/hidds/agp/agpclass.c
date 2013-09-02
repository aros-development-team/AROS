/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/agp.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "agp_private.h"

struct ClassName
{
    struct Node node;
    STRPTR      name;
};

/* HELPERS */
VOID HiddAgpRegisterClass(struct HIDDAGPData * adata, STRPTR class)
{
    struct ClassName * cn = AllocVec(sizeof(struct ClassName), MEMF_ANY | MEMF_CLEAR);
    cn->name = class;
    AddTail(&adata->classes, (struct Node *)cn);
}

/* PUBLIC METHODS */
OOP_Object * METHOD(AGP, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if (o)
    {
        struct HIDDAGPData * adata = OOP_INST_DATA(cl, o);
        NEWLIST(&adata->classes);
        HiddAgpRegisterClass(adata, CLID_Hidd_i845BridgeDevice);
        HiddAgpRegisterClass(adata, CLID_Hidd_i7505BridgeDevice);
        HiddAgpRegisterClass(adata, CLID_Hidd_SiSBridgeDevice);
        HiddAgpRegisterClass(adata, CLID_Hidd_VIABridgeDevice);
        HiddAgpRegisterClass(adata, CLID_Hidd_VIAAgp3BridgeDevice);
        HiddAgpRegisterClass(adata, CLID_Hidd_SiSAgp3BridgeDevice);
    }

    return o;
}

VOID AGP__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg mag)
{
    struct HIDDAGPData * adata = OOP_INST_DATA(cl, o);
    struct ClassName * name = NULL;
    
    /* Free registered class information */
    while((name = (struct ClassName *)RemHead(&adata->classes)) != NULL)
        FreeVec(name);

}

OOP_Object * METHOD(AGP, Hidd_AGP, GetBridgeDevice)
{
    /* Find bridge device matching hardware */
    if (!SD(cl)->bridgedevice)
    {
        struct ClassName * name = NULL;        
        struct HIDDAGPData * adata = OOP_INST_DATA(cl, o);

        ForeachNode(&adata->classes, name)
        {
            struct pHidd_AGPBridgeDevice_Initialize imsg = {
            mID : OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_Initialize)
            };

            OOP_Object * bridgedevice = OOP_NewObject(NULL, name->name, NULL);
        
            if ((BOOL)OOP_DoMethod(bridgedevice, (OOP_Msg)&imsg))
                SD(cl)->bridgedevice = bridgedevice;
            else
                OOP_DisposeObject(bridgedevice);
        }

    }

    return SD(cl)->bridgedevice;
}

