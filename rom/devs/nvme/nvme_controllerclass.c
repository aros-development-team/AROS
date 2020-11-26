/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/utility.h>
#include <proto/alib.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/nvme.h>
#include <hidd/storage.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <stdio.h>

#include "nvme_intern.h"
#include "nvme_hw.h"
#include "nvme_queue.h"
#include "nvme_queue_admin.h"

/*
    NVME_AdminIntCode:
        handle incomming completion event interrupts
        for the controllers Admin queue.
*/
static AROS_INTH1(NVME_AdminIntCode, struct nvme_queue *, nvmeq)
{
    AROS_INTFUNC_INIT

    D(bug ("[NVME:Controller] %s(0x%p)\n", __func__, nvmeq);)

    nvme_process_cq(nvmeq);
    
    return FALSE;

    AROS_INTFUNC_EXIT
}

/* Controller class methods */
OOP_Object *NVME__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct NVMEBase *NVMEBase = (struct NVMEBase *)cl->UserData;

    device_t dev = (device_t)GetTagData(aHidd_DriverData, 0, msg->attrList);
    int cpucnt = 1; // TODO: get the correct count;

    OOP_Object *nvmeController = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (nvmeController)
    {
        struct nvme_Controller *data = OOP_INST_DATA(cl, nvmeController);
        struct nvme_queue *nvmeq;

        D(bug ("[NVME:Controller] Root__New: New Controller Obj @ %p\n", nvmeController);)

        /*register the controller in nvme.device */
        D(bug ("[NVME:Controller] Root__New:     Controller Entry @ 0x%p\n", data);)
        D(bug ("[NVME:Controller] Root__New:     Controller Data @ 0x%p\n", dev);)

        data->ac_Class = cl;
        data->ac_Object = nvmeController;
        if ((data->ac_dev = dev) != NULL)
        {
            dev->dev_Controller = nvmeController;

            OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_Base0, (IPTR *)&dev->dev_nvmeregbase);

            D(bug ("[NVME:Controller] Root__New:     NVME RegBase @ 0x%p\n", dev->dev_nvmeregbase);)
            dev->dbs = ((void volatile *)dev->dev_nvmeregbase) + 4096;

            dev->dev_Queues = AllocMem(sizeof(APTR) * (cpucnt + 1), MEMF_CLEAR);

            D(bug ("[NVME:Controller] Root__New:     dbs @ 0x%p\n", dev->dbs);)
            dev->dev_Queues[0] = nvme_alloc_queue(dev, 0, 64, 0);
            D(bug ("[NVME:Controller] Root__New:     admin queue @ 0x%p\n", dev->dev_Queues[0]);)
            if (dev->dev_Queues[0])
            {
                unsigned long timeout, cmdno;
                UQUAD cap;
                ULONG aqa;
                
                dev->dev_Queues[0]->cehooks = AllocMem(sizeof(_NVMEQUEUE_CE_HOOK) * 16, MEMF_CLEAR);
                dev->dev_Queues[0]->cehandlers = AllocMem(sizeof(struct completionevent_handler *) * 16, MEMF_CLEAR);

                aqa = dev->dev_Queues[0]->q_depth - 1;
                aqa |= aqa << 16;

                dev->ctrl_config = NVME_CC_ENABLE | NVME_CC_CSS_NVM;
//                dev->ctrl_config |= (PAGE_SHIFT - 12) << NVME_CC_MPS_SHIFT;
                dev->ctrl_config |= NVME_CC_ARB_RR | NVME_CC_SHN_NONE;
                dev->ctrl_config |= NVME_CC_IOSQES | NVME_CC_IOCQES;

                dev->dev_nvmeregbase->cc = 0;
                dev->dev_nvmeregbase->aqa = aqa;
                dev->dev_nvmeregbase->asq = (UQUAD)dev->dev_Queues[0]->sqba;
                dev->dev_nvmeregbase->acq = (UQUAD)dev->dev_Queues[0]->cqba;
                dev->dev_nvmeregbase->cc = dev->ctrl_config;

                /* parse capabilities ... */
                cap = dev->dev_nvmeregbase->cap;
#if (0)                
                timeout = ((NVME_CAP_TIMEOUT(cap) + 1) * HZ / 2) + jiffies;
#endif
                dev->db_stride = NVME_CAP_STRIDE(cap);
                D(bug ("[NVME:Controller] Root__New:     doorbell stride = %u\n", dev->db_stride);)

                // add interrupt
                OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_INTLine, &data->ac_PCIIntLine);
                dev->dev_Queues[0]->q_IntHandler.is_Node.ln_Name = "NVME Controller Interrupt";
                dev->dev_Queues[0]->q_IntHandler.is_Node.ln_Pri = 5;
                dev->dev_Queues[0]->q_IntHandler.is_Code = (VOID_FUNC) NVME_AdminIntCode;
                dev->dev_Queues[0]->q_IntHandler.is_Data = dev->dev_Queues[0];
                AddIntServer(INTB_KERNEL + data->ac_PCIIntLine,
                    &dev->dev_Queues[0]->q_IntHandler);

                APTR buffer = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, 8192);
                if (buffer)
                {
                    struct completionevent_handler contrehandle;
                    struct nvme_id_ctrl *ctrl = (struct nvme_id_ctrl *)buffer;
                    struct nvme_command c;

                    contrehandle.ceh_Task = FindTask(NULL);
                    contrehandle.ceh_SigSet = SIGF_SINGLE;

                    memset(&c, 0, sizeof(c));
                    c.identify.op.opcode = nvme_admin_identify;
                    c.identify.nsid = 0;
                    c.identify.prp1 = (UQUAD)buffer;
                    c.identify.cns = 1;

                    D(bug ("[NVME:Controller] Root__New: sending nvme_admin_identify\n");)
                    nvme_submit_admincmd(dev, &c, &contrehandle);
                    Wait(contrehandle.ceh_SigSet);

                    if (!contrehandle.ceh_Status)
                    {
                        D(bug ("[NVME:Controller] Root__New:     Model '%s'\n", ctrl->mn);)
                        D(bug ("[NVME:Controller] Root__New:     Serial '%s'\n", ctrl->sn);)
                        D(bug ("[NVME:Controller] Root__New:     F/W '%s'\n", ctrl->fr);)
                        D(bug ("[NVME:Controller] Root__New:        %u namespace(s)\n", ctrl->nn);)

                        struct TagItem attrs[] =
                        {
                                {aHidd_Name,                (IPTR)"nvme.device"                             },
                                {aHidd_HardwareName,        0                                               },
#define BUS_TAG_HARDWARENAME 1
                                {aHidd_Producer,            GetTagData(aHidd_Producer, 0, msg->attrList)    },
                                {aHidd_Product,             GetTagData(aHidd_Product, 0, msg->attrList)     },
                                {aHidd_DriverData,          (IPTR)dev                                       },
                                {aHidd_Bus_MaxUnits,        ctrl->nn                                        },
                                {aHidd_StorageUnit_Model,   (IPTR)ctrl->mn                                  },
                                {aHidd_StorageUnit_Serial,  (IPTR)ctrl->sn                                  },
                                {aHidd_StorageUnit_Revision,(IPTR)ctrl->fr                                  },
                                {TAG_DONE,                  0                                               }
                        };
                        attrs[BUS_TAG_HARDWARENAME].ti_Data = (IPTR)AllocVec(24, MEMF_CLEAR);
                        sprintf((char *)attrs[BUS_TAG_HARDWARENAME].ti_Data, "NVME %u.%u Device Bus", (dev->dev_nvmeregbase->vs >> 16) & 0xFFFF, dev->dev_nvmeregbase->vs & 0xFFFF);
                        HW_AddDriver(dev->dev_Controller, NVMEBase->busClass, attrs);
                    }
                    else
                    {
                        bug("[NVME:Controller] Root__New: ERROR - failed to query controller identity!\n");
                    }
                    HIDD_PCIDriver_FreePCIMem(dev->dev_PCIDriverObject, buffer);
                }
                else
                {
                    D(bug ("[NVME:Controller] Root__New: ERROR - failed to create DMA buffer!\n");)
                }
            }
            else
            {
                bug("[NVME:Controller] Root__New: ERROR - failed to create Admin Queue!\n");
            }
        }
        else
        {
            bug("[NVME:Controller] Root__New: ERROR - device data missing!\n");
        }

        AddTail(&NVMEBase->nvme_Controllers, &data->ac_Node);
    }
    D(bug ("[NVME:Controller] Root__New: returning 0x%p\n", nvmeController);)
    return nvmeController;
}

VOID NVME__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    struct nvme_Controller *nvmeNode, *tmpNode;
    D(bug ("[NVME:Controller] Root__Dispose(0x%p)\n", o);)
    ForeachNodeSafe (&NVMEBase->nvme_Controllers, nvmeNode, tmpNode)
    {
        if (nvmeNode->ac_Object == o)
        {
            D(bug ("[NVME:Controller] Root__Dispose: Destroying Controller Entry @ %p\n", nvmeNode);)
            Remove(&nvmeNode->ac_Node);
        }
    }
}

BOOL NVME__Hidd_StorageController__RemoveBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_RemoveBus *Msg)
{
    D(bug ("[NVME:Controller] Hidd_StorageController__RemoveBus(0x%p)\n", o);)
   /*
     * Currently we don't support unloading NVME bus drivers.
     * This is a very-very big TODO.
     */
    return FALSE;
}

BOOL NVME__Hidd_StorageController__SetUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_SetUpBus *Msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;

    D(bug ("[NVME:Controller] Hidd_StorageController__SetUpBus(0x%p)\n", Msg->busObject);)

    /* Add the bus to the device and start service */
    return Hidd_NVMEBus_Start(Msg->busObject, NVMEBase);
}

void NVME__Hidd_StorageController__CleanUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_CleanUpBus *msg)
{
    D(bug ("[NVME:Controller] Hidd_StorageController__CleanUpBus(0x%p)\n", o);)
    /* By default we have nothing to do here */
}
