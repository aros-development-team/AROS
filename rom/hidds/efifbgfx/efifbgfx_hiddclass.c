/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Gfx class for the EFI framebuffer.
*/

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

#include "efifbgfx_hidd.h"
#include "efifbgfx_support.h"

#include LC_LIBDEFS_FILE

AROS_INTH1(ResetHandler, struct HWData *, hwdata)
{
    AROS_INTFUNC_INIT

    ClearBuffer(hwdata);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * The framebuffer is memory of whatever display hardware the firmware
 * drove. Where that is a PCI device it is the one with the framebuffer
 * inside one of its regions, and the driver names it as the device it
 * operates. A native driver for the same hardware names the same device,
 * so the two are easy to tell as rivals, and whoever looks at this driver
 * finds what is really behind it.
 */
struct EFIFBGfxDeviceSearch
{
    OOP_AttrBase    pciDeviceAB;
    IPTR            framebuffer;
    OOP_Object      *device;
};

AROS_UFH3(static void, EFIFBGfx_MatchDevice,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, device, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    struct EFIFBGfxDeviceSearch *search = hook->h_Data;
    OOP_AttrBase HiddPCIDeviceAttrBase = search->pciDeviceAB;
    OOP_Object *driver = NULL;
    ULONG i;

    if (search->device)
        return;

    OOP_GetAttr(device, aHidd_PCIDevice_Driver, (IPTR *)&driver);
    if (!driver)
        return;

    for (i = 0; i < 6; i++)
    {
        IPTR base = 0, size = 0;

        /* Base, size and type of the regions follow each other */
        OOP_GetAttr(device, aHidd_PCIDevice_Base0 + i * 3, &base);
        OOP_GetAttr(device, aHidd_PCIDevice_Size0 + i * 3, &size);
        if (!base || !size)
            continue;

        /* The framebuffer is known by the address the CPU reaches it at */
        base = (IPTR)HIDD_PCIDriver_PCItoCPU(driver, (APTR)base);
        if ((search->framebuffer >= base) && (search->framebuffer - base < size))
        {
            search->device = device;
            break;
        }
    }

    AROS_USERFUNC_EXIT
}

static void EFIFBGfx_DeclareDevice(OOP_Class *cl, OOP_Object *o)
{
    OOP_Object *pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

    if (pci)
    {
        struct EFIFBGfxDeviceSearch search =
        {
            OOP_ObtainAttrBase(IID_Hidd_PCIDevice),
            (IPTR)XSD(cl)->data.framebuffer,
            NULL
        };
        struct Hook hook =
        {
            .h_Entry = (HOOKFUNC)EFIFBGfx_MatchDevice,
            .h_Data  = &search
        };

        if (search.pciDeviceAB)
        {
            HIDD_PCI_EnumDevices(pci, &hook, NULL);
            if (search.device)
                HIDD_AddRelation(o, vHidd_Relation_Device, search.device, NULL);

            OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
        }
        OOP_DisposeObject(pci);
    }
}

OOP_Object *EFIFBGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem msgNewTags[] =
    {
        { aHidd_Name            , (IPTR)"efifbgfx.hidd"                },
        { aHidd_HardwareName    , (IPTR)"EFI Framebuffer"            },
        { aHidd_ProducerName    , (IPTR)"UEFI"                       },
        {TAG_MORE, 0UL}
    };
    struct pRoot_New msgNew;

    EnterFunc(bug("EFIFBGfx::New()\n"));

    /* There is one firmware framebuffer; refuse a second driver object */
    if (XSD(cl)->efifbgfxhidd)
        return NULL;

    if ((msgNewTags[3].ti_Data = (IPTR)msg->attrList) == 0)
        msgNewTags[3].ti_Tag = TAG_DONE;

    msgNew.mID = msg->mID;
    msgNew.attrList = msgNewTags;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&msgNew);
    if (o)
    {
        struct EFIFBGfxHiddData *data = OOP_INST_DATA(cl, o);
        struct TagItem displaytags[] =
        {
            { aHidd_Display_GfxHidd, (IPTR)o },
            { TAG_DONE,              0       }
        };

        D(bug("Got object from super\n"));

        XSD(cl)->efidisplay = OOP_NewObject(XSD(cl)->displayclass, NULL, displaytags);
        if (XSD(cl)->efidisplay)
        {
            D(bug("[EFIFBGfx:Driver] %s: display @ 0x%p\n", __func__, XSD(cl)->efidisplay));
            XSD(cl)->efifbgfxhidd = o;

            data->ResetInterrupt.is_Node.ln_Name = cl->ClassNode.ln_Name;
            data->ResetInterrupt.is_Code = (VOID_FUNC)ResetHandler;
            data->ResetInterrupt.is_Data = &XSD(cl)->data;
            AddResetCallback(&data->ResetInterrupt);

            EFIFBGfx_DeclareDevice(cl, o);
        }
        else
        {
            OOP_MethodID dispose_mid = XSD(cl)->mid_Dispose;
            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            o = NULL;
        }
    }
    ReturnPtr("EFIFBGfx::New", OOP_Object *, o);
}

VOID EFIFBGfx__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct EFIFBGfxHiddData *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->ResetInterrupt);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    XSD(cl)->efifbgfxhidd = NULL;
}

VOID EFIFBGfx__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Gfx_NoFrameBuffer:
                *msg->storage = TRUE;
                return;

            case aoHidd_Gfx_DisplayDefault:
                *msg->storage = (IPTR)XSD(cl)->efidisplay;
                return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
