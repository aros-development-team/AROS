/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: A device sitting on a device-tree described PCI host bridge.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/pci.h>
#include <utility/tagitem.h>

#include LC_LIBDEFS_FILE
#include "pcidt.h"

OOP_Object *PCIDTDev__Root__New(OOP_Class *cl, OOP_Object *o,
                                struct pRoot_New *msg)
{
    struct pcidt_staticdata *psd = PSD(cl);
    struct pRoot_New pcidevNew;
    struct TagItem pcidevTags[] =
    {
        { aHidd_Name,                       (IPTR)"pcidt.hidd"  },
        { aHidd_PCIDevice_ExtendedConfig,   0                   },
        { TAG_DONE,                         0                   }
    };
    OOP_Object *driver;
    struct PCIDTBusData *bdata;
    UBYTE bus, dev, sub;
    OOP_Object *deviceObj;
    IPTR mmconfig = 0;

    driver = (OOP_Object *)GetTagData(aHidd_PCIDevice_Driver, 0,
                                      msg->attrList);
    bus = (UBYTE)GetTagData(aHidd_PCIDevice_Bus, 0, msg->attrList);
    dev = (UBYTE)GetTagData(aHidd_PCIDevice_Dev, 0, msg->attrList);
    sub = (UBYTE)GetTagData(aHidd_PCIDevice_Sub, 0, msg->attrList);

    if (!driver)
        return NULL;

    bdata = OOP_INST_DATA(psd->pcidtDriverClass, driver);

    pcidevNew.mID      = msg->mID;
    pcidevNew.attrList = pcidevTags;

    if (msg->attrList)
    {
        pcidevTags[2].ti_Tag  = TAG_MORE;
        pcidevTags[2].ti_Data = (IPTR)msg->attrList;
    }

    /*
     * Extended (4KiB) configuration space is only directly addressable
     * on a flat ECAM bridge. A DesignWare viewport has to be re-aimed
     * for every access, so there is no fixed address to hand out and
     * everything goes through the driver's methods instead.
     */
    if (bdata->bridge.access == PCIDT_ACCESS_ECAM &&
        bus >= bdata->bridge.busStart && bus <= bdata->bridge.busEnd)
    {
        IPTR off = ((IPTR)(bus - bdata->bridge.busStart) << 20) |
                   ((IPTR)(dev & 31) << 15) | ((IPTR)(sub & 7) << 12);

        if (off < bdata->bridge.cfgSize)
        {
            mmconfig = bdata->bridge.cfgBase + off;
            pcidevTags[1].ti_Data = mmconfig;
        }
    }

    /*
     * Whatever firmware left enabled is not ours. A driver asks for
     * messages explicitly through ObtainVectors, and until something
     * takes the device neither kind of interrupt should reach us -
     * a pin is shared by everything below the bridge, so one device
     * nobody services holds the line down for the rest.
     */
    PCIDT_DisableMSI(&bdata->bridge, bus, dev, sub);
    PCIDT_SetINTx(&bdata->bridge, bus, dev, sub, FALSE);

    deviceObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, &pcidevNew.mID);
    if (deviceObj)
    {
        struct PCIDTDeviceData *data = OOP_INST_DATA(cl, deviceObj);

        data->mmconfig = (APTR)mmconfig;
        data->bus = bus;
        data->dev = dev;
        data->sub = sub;
        data->bridge = bdata->bridge.index;
        data->msiVector = -1;

        D(bug("[PCIDT:Device] %s: %02x:%02x.%x config @ 0x%p\n", __func__,
              bus, dev, sub, mmconfig);)
    }

    return deviceObj;
}


/*
 * Message interrupts for one device.
 *
 * There is a single source per controller and a status register saying
 * which vector arrived, so a vector is a bit in that register and the
 * "interrupt" a driver is given is the controller's own source. Drivers
 * sharing it are told apart by looking at their own hardware, which is
 * what they do for a shared pin anyway.
 */
BOOL PCIDTDev__Hidd_PCIDevice__ObtainVectors(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDevice_ObtainVectors *msg)
{
    struct pcidt_staticdata *psd = PSD(cl);
    struct PCIDTDeviceData *data = OOP_INST_DATA(cl, o);
    struct pcidt_bridge *b;
    LONG vector;

    if (data->bridge >= psd->bridgeCount)
        return FALSE;

    b = &psd->bridges[data->bridge];

    if (!b->msiReady)
    {
        D(bug("[PCIDT:msi] %02x:%02x.%x: no message controller here\n",
              data->bus, data->dev, data->sub);)
        return FALSE;
    }

    vector = PCIDT_MSIEnable(b, data->bus, data->dev, data->sub);
    if (vector < 0)
        return FALSE;

    data->msiVector = vector;

    /* On a message it drives no pin, and must not be left able to */
    PCIDT_SetINTx(b, data->bus, data->dev, data->sub, FALSE);

    return TRUE;
}

VOID PCIDTDev__Hidd_PCIDevice__GetVectorAttribs(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDevice_GetVectorAttribs *msg)
{
    struct pcidt_staticdata *psd = PSD(cl);
    struct PCIDTDeviceData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attribs;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case tHidd_PCIVector_Native:
            tag->ti_Data = (data->msiVector >= 0) ? data->msiVector : -1;
            break;

        case tHidd_PCIVector_Int:
            tag->ti_Data = (data->msiVector >= 0 &&
                            data->bridge < psd->bridgeCount)
                           ? psd->bridges[data->bridge].msiIrq : -1;
            break;
        }
    }
}
