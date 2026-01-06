/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/usb.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "pcixhci.h"

OOP_Object *XHCIController__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID XHCIController__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

void XHCIController__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

#   define XHCIController_DATA_SIZE (sizeof(struct XHCIController))

#ifdef base
#undef base
#endif
#define base hd

int XHCIControllerOOPStartup(struct PCIDevice *hd)
{
    if (!hd->hd_USBXHCIControllerClass) {
        OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
        OOP_Class *cl = NULL;

        struct OOP_MethodDescr XHCIController_Root_descr[] = {
            {(OOP_MethodFunc)XHCIController__Root__New, moRoot_New},
            {(OOP_MethodFunc)XHCIController__Root__Dispose, moRoot_Dispose},
            {(OOP_MethodFunc)XHCIController__Root__Get, moRoot_Get},
            {NULL, 0}
        };
    #define NUM_XHCIController_Root_METHODS 3

        struct OOP_InterfaceDescr XHCIController_ifdescr[] = {
            {XHCIController_Root_descr, IID_Root, NUM_XHCIController_Root_METHODS},
            {NULL, NULL}
        };

        struct TagItem XHCIController_tags[] = {
            {aMeta_SuperID, (IPTR)CLID_Hidd_USBController},
            {aMeta_InterfaceDescr, (IPTR)XHCIController_ifdescr},
            {aMeta_InstSize, (IPTR)XHCIController_DATA_SIZE},
            {TAG_DONE, (IPTR)0}
        };

        if (MetaAttrBase == 0)
            return FALSE;

        cl = OOP_NewObject(NULL, CLID_HiddMeta, XHCIController_tags);
        if (cl != NULL) {
            cl->UserData = (APTR)hd;
            hd->hd_USBXHCIControllerClass = cl;
            OOP_AddClass(cl);
        }

        OOP_ReleaseAttrBase(IID_Meta);
    }
    return hd->hd_USBXHCIControllerClass != NULL;
}

static void OOP_XHCIController_Shutdown(struct PCIDevice *hd)
{
    if (hd->hd_USBXHCIControllerClass != NULL) {
        OOP_RemoveClass(hd->hd_USBXHCIControllerClass);
        OOP_DisposeObject((OOP_Object *)hd->hd_USBXHCIControllerClass);
    }
}
