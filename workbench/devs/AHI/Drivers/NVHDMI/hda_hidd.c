/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2026 The AROS Dev Team.

All Rights Reserved.

Glue binding the driver to the hdaudio.hidd base class: a private
subclass implements the moHidd_HDA_HWInit/HWExit overrides that make a
PCI-attached controller work, and the driver finds, owns and wraps the
PCI functions it wants to drive.
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <aros/asmcall.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/hda.h>
#include <hardware/pci.h>

#include "hda_hidd.h"

#define MAX_CONTROLLERS 8

struct Library *OOPBase;
OOP_AttrBase __IHidd_HDA;
OOP_AttrBase __IHidd_PCIDevice;

static struct Library *HDAHiddBase;
static OOP_Class *hdapciClass;

/******************************************************************************
** The PCI attachment subclass ************************************************
******************************************************************************/

struct HDAPCIData
{
    OOP_Object *pciDevice;
    OOP_Object *pciDriver;
    BOOL        irqAdded;
};

/* Controller fixes, mainly cache snoop control */
static void HDAPCI_Quirks(OOP_Object *dev, UWORD vendor, UWORD product)
{
    static const UWORD intel_no_snoop[] =
    {
        0x2668, 0x27d8, 0x269a, 0x284b, 0x293e, 0x293f, 0x3a3e, 0x3a6e, 0
    };
    ULONG data;
    UWORD i;

    /*
     * Traffic Class Select: CORB, RIRB and stream data on TC0
     * (Intel HD Audio specification, PCI configuration register 0x44)
     */
    data = HIDD_PCIDevice_ReadConfigByte(dev, 0x44);
    HIDD_PCIDevice_WriteConfigByte(dev, 0x44, data & ~0x07);

    if (vendor == 0x8086)
    {
        BOOL snoop = TRUE;

        for (i = 0; intel_no_snoop[i]; i++)
        {
            if (intel_no_snoop[i] == product)
                snoop = FALSE;
        }

        if (snoop)
        {
            D(bug("[" DRIVER "] Intel controller, enabling snoop\n"));
            data = HIDD_PCIDevice_ReadConfigWord(dev, 0x78);
            HIDD_PCIDevice_WriteConfigWord(dev, 0x78, data & ~0x800);
        }
    }

    if ((vendor == 0x1002 && (product == 0x437B || product == 0x4383)) ||
        (vendor == 0x1022 && product == 0x780D))
    {
        D(bug("[" DRIVER "] ATI SB/AMD Hudson controller, enabling snoop\n"));
        data = HIDD_PCIDevice_ReadConfigByte(dev, 0x42);
        HIDD_PCIDevice_WriteConfigByte(dev, 0x42, (data & ~0x07) | 0x02);
    }

    if (vendor == 0x10DE)
    {
        D(bug("[" DRIVER "] NVidia controller, enabling snoop\n"));
        data = HIDD_PCIDevice_ReadConfigByte(dev, 0x4E);
        HIDD_PCIDevice_WriteConfigByte(dev, 0x4E, data | 0x0F);
        data = HIDD_PCIDevice_ReadConfigByte(dev, 0x4D);
        HIDD_PCIDevice_WriteConfigByte(dev, 0x4D, data | 0x01);
        data = HIDD_PCIDevice_ReadConfigByte(dev, 0x4C);
        HIDD_PCIDevice_WriteConfigByte(dev, 0x4C, data | 0x01);
    }
}

static OOP_Object *HDAPCI__Root__New(OOP_Class *cl, OOP_Object *o,
        struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HDAPCIData *hpd = OOP_INST_DATA(cl, o);
        IPTR val = 0;

        /* The base class stores our PCI device as the device data */
        OOP_GetAttr(o, aHidd_HDA_DeviceData, &val);
        hpd->pciDevice = (OOP_Object *)val;
        if (hpd->pciDevice)
            OOP_GetAttr(hpd->pciDevice, aHidd_PCIDevice_Driver,
                        (IPTR *)&hpd->pciDriver);

        if (!hpd->pciDevice || !hpd->pciDriver || !HIDD_HDA_Setup(o))
        {
            OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root,
                                                       moRoot_Dispose);

            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            o = NULL;
        }
    }

    return o;
}

static APTR HDAPCI__Hidd_HDA__HWInit(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_HWInit *msg)
{
    struct HDAPCIData *hpd = OOP_INST_DATA(cl, o);
    OOP_Object *dev = hpd->pciDevice;
    IPTR mmio = 0, val = 0, barsize = 0;
    UWORD vendor, product, command;

    OOP_GetAttr(dev, aHidd_PCIDevice_VendorID, &val);
    vendor = val;
    OOP_GetAttr(dev, aHidd_PCIDevice_ProductID, &val);
    product = val;

    D(bug("[" DRIVER "] PCI HDA controller %04x:%04x\n", vendor, product));

    command = HIDD_PCIDevice_ReadConfigWord(dev, PCICS_COMMAND);
    command |= PCICMF_IODECODE | PCICMF_MEMDECODE | PCICMF_BUSMASTER;
    HIDD_PCIDevice_WriteConfigWord(dev, PCICS_COMMAND, command);

    HDAPCI_Quirks(dev, vendor, product);

    OOP_GetAttr(dev, aHidd_PCIDevice_Base0, &mmio);
    OOP_GetAttr(dev, aHidd_PCIDevice_Size0, &barsize);
    if (!mmio)
    {
        D(bug("[" DRIVER "] No BAR0!\n"));
        return NULL;
    }

    /* A BAR outside the identity mapped window has no CPU mapping yet */
    mmio = (IPTR)HIDD_PCIDriver_MapPCI(hpd->pciDriver, (APTR)mmio, (ULONG)barsize);
    if (!mmio)
    {
        D(bug("[" DRIVER "] Failed to map BAR0!\n"));
        return NULL;
    }

    if (!HIDD_PCIDriver_AddInterrupt(hpd->pciDriver, dev, msg->controllerInt))
    {
        D(bug("[" DRIVER "] Failed to add interrupt handler\n"));
        return NULL;
    }
    hpd->irqAdded = TRUE;

    return (APTR)mmio;
}

static VOID HDAPCI__Hidd_HDA__HWExit(OOP_Class *cl, OOP_Object *o,
        struct pHidd_HDA_HWExit *msg)
{
    struct HDAPCIData *hpd = OOP_INST_DATA(cl, o);
    OOP_Object *dev = hpd->pciDevice;
    UWORD command;

    if (hpd->irqAdded)
    {
        HIDD_PCIDriver_RemoveInterrupt(hpd->pciDriver, dev,
                                       msg->controllerInt);
        hpd->irqAdded = FALSE;
    }

    command = HIDD_PCIDevice_ReadConfigWord(dev, PCICS_COMMAND);
    command &= ~(PCICMF_IODECODE | PCICMF_MEMDECODE | PCICMF_BUSMASTER);
    HIDD_PCIDevice_WriteConfigWord(dev, PCICS_COMMAND, command);
}

static BOOL create_pci_subclass(void)
{
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct OOP_MethodDescr root_descr[] =
    {
        {(OOP_MethodFunc)HDAPCI__Root__New, moRoot_New},
        {NULL, 0}
    };

    struct OOP_MethodDescr hda_descr[] =
    {
        {(OOP_MethodFunc)HDAPCI__Hidd_HDA__HWInit, moHidd_HDA_HWInit},
        {(OOP_MethodFunc)HDAPCI__Hidd_HDA__HWExit, moHidd_HDA_HWExit},
        {NULL, 0}
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr, IID_Root,     1},
        {hda_descr,  IID_Hidd_HDA, 2},
        {NULL, NULL}
    };

    struct TagItem tags[] =
    {
        {aMeta_SuperID,        (IPTR)CLID_Hidd_HDA},
        {aMeta_InterfaceDescr, (IPTR)ifdescr},
        {aMeta_InstSize,       (IPTR)sizeof(struct HDAPCIData)},
        {TAG_DONE,             0}
    };

    if (!MetaAttrBase)
        return FALSE;

    hdapciClass = OOP_NewObject(NULL, CLID_HiddMeta, tags);

    OOP_ReleaseAttrBase(IID_Meta);

    return hdapciClass != NULL;
}

/******************************************************************************
** Controller discovery *******************************************************
******************************************************************************/

struct pci_enum_ctx
{
    BOOL displayAudio;
    UWORD vendor;
    OOP_Object *devices[MAX_CONTROLLERS];
    ULONG count;
};

/*
    The audio function of a display adapter: a function other than 0
    whose function 0 is a display device.
*/
static BOOL device_is_display_audio(OOP_Object *dev)
{
    OOP_Object *driver = NULL;
    IPTR sub = 0, bus = 0, devnr = 0;
    UWORD classword;

    OOP_GetAttr(dev, aHidd_PCIDevice_Sub, &sub);
    if (sub == 0)
        return FALSE;

    OOP_GetAttr(dev, aHidd_PCIDevice_Driver, (IPTR *)&driver);
    if (!driver)
        return FALSE;

    OOP_GetAttr(dev, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(dev, aHidd_PCIDevice_Dev, &devnr);

    classword = HIDD_PCIDriver_ReadConfigWord(driver, dev, bus, devnr, 0,
                                              0x0A);

    return ((classword >> 8) & 0xFF) == 0x03;
}

AROS_UFH3(static void, PCIEnumerator,
          AROS_UFHA(struct Hook *, hook, A0),
          AROS_UFHA(OOP_Object *, device, A2),
          AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct pci_enum_ctx *ctx = (struct pci_enum_ctx *)hook->h_Data;
    IPTR val = 0;

    if (ctx->count >= MAX_CONTROLLERS)
        return;

    if (ctx->vendor)
    {
        OOP_GetAttr(device, aHidd_PCIDevice_VendorID, &val);
        if ((UWORD)val != ctx->vendor)
            return;
    }

    if (device_is_display_audio(device) != ctx->displayAudio)
        return;

    /* Bind: claim the PCI function for this driver */
    if (HIDD_PCIDevice_Obtain(device, DRIVER) != NULL)
    {
        D(bug("[" DRIVER "] device @ 0x%p already owned\n", device));
        return;
    }

    ctx->devices[ctx->count++] = device;

    AROS_USERFUNC_EXIT
}

ULONG ahi_hda_obtain_controllers(BOOL displayAudio, UWORD vendor,
                                 APTR *controllers, ULONG max)
{
    struct pci_enum_ctx ctx = { displayAudio, vendor, {NULL}, 0 };

    struct Hook enumHook =
    {
        .h_Entry = (HOOKFUNC)PCIEnumerator,
        .h_Data  = &ctx
    };

    struct TagItem requirements[] =
    {
        { tHidd_PCI_Class,    0x04 }, /* Multimedia */
        { tHidd_PCI_SubClass, 0x03 }, /* HD Audio */
        { TAG_DONE,           0    }
    };

    OOP_Object *pci;
    ULONG i, count = 0;

    if (!hdapciClass)
        return 0;

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (!pci)
        return 0;

    HIDD_PCI_EnumDevices(pci, &enumHook, requirements);

    OOP_DisposeObject(pci);

    for (i = 0; i < ctx.count; i++)
    {
        struct TagItem tags[] =
        {
            { aHidd_HDA_DeviceData, (IPTR)ctx.devices[i] },
            { TAG_DONE,             0                    }
        };
        OOP_Object *ctrl;

        if (count < max &&
            (ctrl = OOP_NewObject(hdapciClass, NULL, tags)) != NULL)
        {
            controllers[count++] = ctrl;
        }
        else
        {
            HIDD_PCIDevice_Release(ctx.devices[i]);
        }
    }

    return count;
}

void ahi_hda_release_controller(APTR controller)
{
    IPTR val = 0;
    OOP_Object *dev;

    if (!controller)
        return;

    OOP_GetAttr((OOP_Object *)controller, aHidd_HDA_DeviceData, &val);
    dev = (OOP_Object *)val;

    OOP_DisposeObject((OOP_Object *)controller);

    if (dev)
        HIDD_PCIDevice_Release(dev);
}

/******************************************************************************
** Library setup and call-throughs ********************************************
******************************************************************************/

BOOL ahi_hda_init(struct DriverBase *AHIsubBase)
{
    OOPBase = OpenLibrary(AROSOOP_NAME, 0);
    if (!OOPBase)
        return FALSE;

    HDAHiddBase = OpenLibrary("DRIVERS:hdaudio.hidd", 0);
    if (!HDAHiddBase)
    {
        D(bug("[" DRIVER "] Cannot open hdaudio.hidd\n"));
        return FALSE;
    }

    __IHidd_HDA = OOP_ObtainAttrBase(IID_Hidd_HDA);
    __IHidd_PCIDevice = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (!__IHidd_HDA || !__IHidd_PCIDevice)
        return FALSE;

    if (!create_pci_subclass())
    {
        D(bug("[" DRIVER "] Cannot create PCI attachment subclass\n"));
        return FALSE;
    }

    return TRUE;
}

void ahi_hda_exit(void)
{
    if (hdapciClass)
    {
        OOP_DisposeObject((OOP_Object *)hdapciClass);
        hdapciClass = NULL;
    }

    if (__IHidd_HDA)
    {
        OOP_ReleaseAttrBase(IID_Hidd_HDA);
        __IHidd_HDA = 0;
    }

    if (__IHidd_PCIDevice)
    {
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
        __IHidd_PCIDevice = 0;
    }

    if (HDAHiddBase)
    {
        CloseLibrary(HDAHiddBase);
        HDAHiddBase = NULL;
    }

    if (OOPBase)
    {
        CloseLibrary(OOPBase);
        OOPBase = NULL;
    }
}

IPTR ahi_hda_get_attr(APTR controller, ULONG attr)
{
    IPTR val = 0;

    OOP_GetAttr((OOP_Object *)controller, attr, &val);

    return val;
}

ULONG hda_command(APTR controller, ULONG cmd)
{
    return HIDD_HDA_SendCommand((OOP_Object *)controller, cmd);
}

APTR hda_stream_alloc(APTR controller, ULONG direction,
                      struct Interrupt *streamInt)
{
    return HIDD_HDA_AllocStream((OOP_Object *)controller, direction,
                                streamInt);
}

BOOL hda_stream_setup(APTR controller, APTR stream, UWORD format,
                      ULONG bufferSize, ULONG bufferCount,
                      struct HDA_StreamInfo *info)
{
    return HIDD_HDA_SetupStream((OOP_Object *)controller, stream, format,
                                bufferSize, bufferCount, info);
}

void hda_stream_start(APTR controller, APTR stream)
{
    HIDD_HDA_StartStream((OOP_Object *)controller, stream);
}

void hda_stream_stop(APTR controller, APTR stream)
{
    HIDD_HDA_StopStream((OOP_Object *)controller, stream);
}

void hda_stream_free(APTR controller, APTR stream)
{
    HIDD_HDA_FreeStream((OOP_Object *)controller, stream);
}

void hda_stream_sync(APTR controller, APTR stream, ULONG index)
{
    HIDD_HDA_SyncStreamBuffer((OOP_Object *)controller, stream, index);
}
