/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Intel VMD PCI bus driver class.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "pcivmd.h"

const char pcivmdHWName[] = "Intel Volume Management Device PCI Express Controller";

OOP_Object *PCIVMD__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pcivmd_staticdata *psd = PSD(cl);
    struct VMDDomain *dom = (struct VMDDomain *)GetTagData(aHidd_DriverData, 0, msg->attrList);
    struct pRoot_New ncMsg;
    struct TagItem ncTags[] =
    {
        { aHidd_Name,           (IPTR)"pcivmd.hidd"     },
        { aHidd_HardwareName,   (IPTR)pcivmdHWName      },
        { TAG_MORE,             (IPTR)msg->attrList     }
    };
    OOP_Object *driver;

    if (!dom)
        return NULL;

    ncMsg.mID      = msg->mID;
    ncMsg.attrList = ncTags;

    driver = (OOP_Object *)OOP_DoSuperMethod(cl, o, &ncMsg.mID);
    if (driver)
    {
        struct VMDBusData *data = OOP_INST_DATA(cl, driver);

        data->domain = dom;
    }
    D(bug("[PCIVMD:Driver] %s: returning 0x%p\n", __func__, driver);)

    return driver;
}

void PCIVMD__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct pcivmd_staticdata *psd = PSD(cl);
    ULONG idx;

    if (IS_PCIDRV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_PCIDriver_DeviceClass:
                *msg->storage = (IPTR)psd->vmdDeviceClass;
                return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

UBYTE PCIVMD__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    struct VMDBusData *data = OOP_INST_DATA(cl, o);

    return (UBYTE)VMD_ReadConfig(data->domain, msg->bus, msg->dev, msg->sub, msg->reg, 1);
}

UWORD PCIVMD__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    struct VMDBusData *data = OOP_INST_DATA(cl, o);

    return (UWORD)VMD_ReadConfig(data->domain, msg->bus, msg->dev, msg->sub, msg->reg, 2);
}

ULONG PCIVMD__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    struct VMDBusData *data = OOP_INST_DATA(cl, o);

    return VMD_ReadConfig(data->domain, msg->bus, msg->dev, msg->sub, msg->reg, 4);
}

void PCIVMD__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    struct VMDBusData *data = OOP_INST_DATA(cl, o);

    VMD_WriteConfig(data->domain, msg->bus, msg->dev, msg->sub, msg->reg, 1, msg->val);
}

void PCIVMD__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    struct VMDBusData *data = OOP_INST_DATA(cl, o);

    VMD_WriteConfig(data->domain, msg->bus, msg->dev, msg->sub, msg->reg, 2, msg->val);
}

void PCIVMD__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    struct VMDBusData *data = OOP_INST_DATA(cl, o);

    VMD_WriteConfig(data->domain, msg->bus, msg->dev, msg->sub, msg->reg, 4, msg->val);
}

/*
    PCIDriver::MapPCI(Address, Length) - the devices live in the windows
    of the VMD, which belong to the bus it is on. Let that bus do the
    mapping.
*/
APTR PCIVMD__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_MapPCI *msg)
{
    struct VMDBusData *data = OOP_INST_DATA(cl, o);

    return VMD_MapBus(data->domain, (UQUAD)(IPTR)msg->PCIAddress, msg->Length);
}

static void vmdIRQwrapper(void *data1, void *data2)
{
    struct Interrupt *irq = (struct Interrupt *)data1;

    AROS_INTC1(irq->is_Code, irq->is_Data);
}

/*
    PCIDriver::AddInterrupt(device, interrupt) - there is no interrupt
    pin to listen on behind a VMD. A driver that asks for one is quietly
    given the device's first message vector instead, which arrives with
    whatever else shares the VMD vector it is routed to - exactly as a
    shared pin would.
*/
BOOL PCIVMD__Hidd_PCIDriver__AddInterrupt(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_AddInterrupt *msg)
{
    struct pcivmd_staticdata *psd = PSD(cl);
    struct VMDBusData *data = OOP_INST_DATA(cl, o);
    struct VMDDeviceData *devdata = OOP_INST_DATA(psd->vmdDeviceClass, msg->device);

    if ((devdata->vecMode == VMDVEC_NONE) && !VMD_ObtainVectors(devdata, 1, 1))
    {
        bug("[PCIVMD:Driver] %02x:%02x.%x has no interrupt it can use\n",
            devdata->bus, devdata->dev, devdata->sub);
        return FALSE;
    }

    msg->interrupt->is_Node.ln_Succ =
        KrnAddIRQHandler(data->domain->vd_Vec[devdata->vecMap[0]].irq,
                         vmdIRQwrapper, msg->interrupt, NULL);

    return msg->interrupt->is_Node.ln_Succ ? TRUE : FALSE;
}
