/*
    Copyright (C) 2020-2023, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
/* We want all other bases obtained from our base */
#define __NOLIBBASE__
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/nvme.h>
#include <hidd/storage.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <string.h>

#include "nvme_debug.h"
#include "nvme_intern.h"
#include "nvme_timer.h"
#include "nvme_hw.h"
#include "nvme_queue.h"
#include "nvme_queue_admin.h"

#ifndef PAGESHIFT
#define PAGESHIFT 12
#endif

#define DIRQ(x) x

/*
    By rights, we should disable the nvme controller completly on shutdown
    but doing so causes problems (we need to wait for the irq, which lets other things run)
    and crashes the shutdown process eventually.
*/
//#define DISABLE_CONTROLLER_ONSHUTDOWN

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

    D(bug ("[NVME:Controller] %s: finished\n", __func__);)

    return TRUE;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(NVME_ResetHandler, device_t, dev)
{
    AROS_INTFUNC_INIT

    struct completionevent_handler cehandle;
    struct nvme_command c;
    UWORD qIRQ;

    DIRQ(bug ("[NVME:Controller] %s(0x%p)\n", __func__, dev);)

    memset(&c, 0, sizeof(c));
    memset(&cehandle, 0, sizeof(cehandle));

    cehandle.ceh_Task = FindTask(NULL);
    cehandle.ceh_SigSet = SIGF_SINGLE;

    qIRQ = dev->dev_Queues[0]->q_irq;

    DIRQ(bug ("[NVME:Controller] %s: disabling io queue interrupts\n", __func__);)
    dev->ctrl_config &= ~(NVME_CC_IOSQES | NVME_CC_IOCQES);
    dev->dev_nvmeregbase->cc = dev->ctrl_config;
#if defined(DISABLE_CONTROLLER_ONSHUTDOWN)
    DIRQ(bug ("[NVME:Controller] %s: deleting submission queue\n", __func__);)
	c.delete_queue.op.opcode = nvme_admin_delete_sq;
	c.delete_queue.qid = AROS_WORD2LE(1);
    nvme_submit_admincmd(dev, &c, &cehandle);

    Wait(cehandle.ceh_SigSet);
    if (!cehandle.ceh_Status)
    {
        DIRQ(bug ("[NVME:Controller] %s: deleting completion queue\n", __func__);)
        c.delete_queue.op.opcode = nvme_admin_delete_cq;
        c.delete_queue.qid = AROS_WORD2LE(1);
        nvme_submit_admincmd(dev, &c, &cehandle);
        Wait(cehandle.ceh_SigSet);
    }
#endif
    DIRQ(bug ("[NVME:Controller] %s: disabling controller interrupts\n", __func__);)
    dev->ctrl_config &= ~(NVME_CC_ENABLE | NVME_CC_CSS_NVM);
    dev->ctrl_config &= ~(NVME_CC_ARB_RR | NVME_CC_SHN_NONE);
    dev->dev_nvmeregbase->cc = dev->ctrl_config;

#if defined(DISABLE_CONTROLLER_ONSHUTDOWN)
    if (!cehandle.ceh_Status)
    {
        c.features.op.opcode = nvme_admin_set_features;
        c.features.fid = AROS_LONG2LE(NVME_FEAT_NUM_QUEUES);
        c.features.dword11 = 0;
#if defined(USE_MSI)
        DIRQ(bug ("[NVME:Controller] %s: disabling nvme MSI capability, and\n", __func__);)
#endif
        DIRQ(bug ("[NVME:Controller] %s: setting queue count to 0\n", __func__);)
        nvme_submit_admincmd(dev, &c, &cehandle);
        Wait(cehandle.ceh_SigSet);
        if (!cehandle.ceh_Status)
        {
            DIRQ(bug ("[NVME:Controller] %s: Controller ready for shutdown\n", __func__);)
        }
    }
#endif
#if defined(USE_MSI)
    struct NVMEBase *NVMEBase = dev->dev_NVMEBase;
    IPTR PCIIntLine = 0;
    OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_INTLine, &PCIIntLine);
    if (PCIIntLine != qIRQ)
    {
        DIRQ(bug ("[NVME:Controller] %s: disabling MSI interrupts\n", __func__);)
        HIDD_PCIDevice_ReleaseVectors(dev->dev_Object);
    }
#endif

    DIRQ(bug ("[NVME:Controller] %s: finished (staus = %08x)\n", __func__, cehandle.ceh_Status);)

    return (cehandle.ceh_Status == 0);

    AROS_INTFUNC_EXIT
}

/* Controller class methods */
OOP_Object *NVME__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct NVMEBase *NVMEBase = (struct NVMEBase *)cl->UserData;
    device_t dev = (device_t)GetTagData(aHidd_DriverData, 0, msg->attrList);
    struct IORequest *nvmeTimer = nvme_OpenTimer(NVMEBase);
    OOP_Object *nvmeController;
 
    if (!nvmeTimer)
    {
        D(bug ("[NVME:Controller] %s: Failed to open time device\n", __func__);)
        return NULL;
    }
    
    nvmeController = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (nvmeController)
    {
        struct TagItem pciActivateBusmaster[] =
        {
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE, 0UL },
        };
        struct nvme_Controller *data = OOP_INST_DATA(cl, nvmeController);
        struct nvme_queue *nvmeq;

        D(bug ("[NVME:Controller] %s: New Controller Obj @ %p\n", __func__, nvmeController);)

        /*register the controller in nvme.device */
        D(
            bug ("[NVME:Controller] %s:     Controller Entry @ 0x%p\n", __func__, data);
            bug ("[NVME:Controller] %s:     Controller Data @ 0x%p\n", __func__, dev);
        )

        data->ac_Class = cl;
        data->ac_Object = nvmeController;
        if ((data->ac_dev = dev) != NULL)
        {
            dev->dev_Controller = nvmeController;
            OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_Base0, (IPTR *)&dev->dev_nvmeregbase);

            D(bug ("[NVME:Controller] %s:     NVME RegBase @ 0x%p\n", __func__, dev->dev_nvmeregbase);)
            dev->dbs = ((void volatile *)dev->dev_nvmeregbase) + 4096;

            dev->dev_Queues = AllocMem(sizeof(APTR) * (KrnGetCPUCount() + 1), MEMF_CLEAR);
            if (!dev->dev_Queues)
            {
                // TODO: dispose the controller object
                nvme_CloseTimer(nvmeTimer);
                return NULL;
            }

            D(bug ("[NVME:Controller] %s:     dbs @ 0x%p\n", __func__, dev->dbs);)
            dev->dev_Queues[0] = nvme_alloc_queue(dev, 0, 64, 0);
            D(bug ("[NVME:Controller] %s:     admin queue @ 0x%p\n", __func__, dev->dev_Queues[0]);)
            if (dev->dev_Queues[0])
            {
                unsigned long timeout, cmdno;
                APTR buffer;
                UQUAD cap;
                ULONG sigs, aqa;

                dev->dev_Queues[0]->cehooks = AllocMem(sizeof(_NVMEQUEUE_CE_HOOK) * 16, MEMF_CLEAR);
                if (!dev->dev_Queues[0]->cehooks)
                {
                    FreeMem(dev->dev_Queues, sizeof(APTR) * (KrnGetCPUCount() + 1));
                    dev->dev_Queues = NULL;
                    // TODO: dispose the controller object
                    nvme_CloseTimer(nvmeTimer);
                    return NULL;
                }
                D(bug ("[NVME:Controller] %s:     admin queue hooks @ 0x%p\n", __func__, dev->dev_Queues[0]->cehooks);)
                dev->dev_Queues[0]->cehandlers = AllocMem(sizeof(struct completionevent_handler *) * 16, MEMF_CLEAR);
                if (!dev->dev_Queues[0]->cehandlers)
                {
                    FreeMem(dev->dev_Queues[0]->cehooks, sizeof(_NVMEQUEUE_CE_HOOK) * 16);
                    FreeMem(dev->dev_Queues, sizeof(APTR) * (KrnGetCPUCount() + 1));
                    dev->dev_Queues = NULL;
                    // TODO: dispose the controller object
                    nvme_CloseTimer(nvmeTimer);
                    return NULL;
                }
                D(bug ("[NVME:Controller] %s:     admin queue handlers @ 0x%p\n", __func__, dev->dev_Queues[0]->cehandlers);)

                aqa = dev->dev_Queues[0]->q_depth - 1;
                aqa |= aqa << 16;

                dev->ctrl_config = NVME_CC_ENABLE | NVME_CC_CSS_NVM;
                dev->ctrl_config |= NVME_CC_ARB_RR | NVME_CC_SHN_NONE;
                dev->ctrl_config |= NVME_CC_IOSQES | NVME_CC_IOCQES;

                dev->dev_nvmeregbase->cc = 0;
                dev->dev_nvmeregbase->aqa = aqa;
                dev->dev_nvmeregbase->asq = (UQUAD)(IPTR)dev->dev_Queues[0]->sqba;
                dev->dev_nvmeregbase->acq = (UQUAD)(IPTR)dev->dev_Queues[0]->cqba;

                /* parse capabilities ... */
                cap = dev->dev_nvmeregbase->cap;

                dev->db_stride = NVME_CAP_STRIDE(cap);
                D(bug ("[NVME:Controller] %s:     doorbell stride = %u\n", __func__, dev->db_stride);)

                dev->pageshift = MIN(MAX(((cap >> 48) & 0xf) + 12, PAGESHIFT),
                    ((cap >> 52) & 0xf) + 12);
                dev->pagesize = 1UL << (dev->pageshift);
                D(bug ("[NVME:Controller] %s: using pagesize %u (%u shift)\n", __func__, dev->pagesize, dev->pageshift);)
                dev->ctrl_config |= (dev->pageshift - 12) << NVME_CC_MPS_SHIFT;
                dev->dev_nvmeregbase->cc = dev->ctrl_config;

                OOP_SetAttrs(dev->dev_Object, (struct TagItem *) pciActivateBusmaster);
                /*
                 * Add the initial admin queue interrupt.
                 * Use the devices IRQ.
                 */
                dev->dev_Queues[0]->q_IntHandler.is_Node.ln_Name = "NVME Controller Interrupt";
                dev->dev_Queues[0]->q_IntHandler.is_Node.ln_Pri = 5;
                dev->dev_Queues[0]->q_IntHandler.is_Code = (VOID_FUNC) NVME_AdminIntCode;
                dev->dev_Queues[0]->q_IntHandler.is_Data = dev->dev_Queues[0];
                HIDD_PCIDriver_AddInterrupt(dev->dev_PCIDriverObject, dev->dev_Object, &dev->dev_Queues[0]->q_IntHandler);

                buffer = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, 8192);
                if (buffer)
                {
                    struct nvme_id_ctrl *ctrl = (struct nvme_id_ctrl *)buffer;
                    struct completionevent_handler cehandle;
                    struct nvme_command c;

                    cehandle.ceh_Task = FindTask(NULL);
                    cehandle.ceh_SigSet = SIGF_SINGLE;

                    memset(&c, 0, sizeof(c));
                    c.identify.op.opcode = nvme_admin_identify;
                    c.identify.nsid = 0;
                    c.identify.prp1 = (UQUAD)(IPTR)buffer;
                    c.identify.cns = 1;

                    D(bug ("[NVME:Controller] %s: sending nvme_admin_identify\n", __func__);)
                    ULONG signals = SetSignal(0, 0);
                    nvme_submit_admincmd(dev, &c, &cehandle);
                    sigs = nvme_WaitTO(nvmeTimer, 1, 0, cehandle.ceh_SigSet);
                    SetSignal(signals, signals);
                    if ((sigs & cehandle.ceh_SigSet) && (!cehandle.ceh_Status))
                    {
                        D(bug ("[NVME:Controller] %s:     Model '%s'\n", __func__, ctrl->mn);)
                        D(bug ("[NVME:Controller] %s:     Serial '%s'\n", __func__, ctrl->sn);)
                        D(bug ("[NVME:Controller] %s:     F/W '%s'\n", __func__, ctrl->fr);)
                        D(bug ("[NVME:Controller] %s:        %u namespace(s)\n", __func__, ctrl->nn);)

                        D(bug ("[NVME:Controller] %s: mdts = %u\n", __func__, ctrl->mdts);)
                        dev->dev_mdts = ctrl->mdts;

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

                        nvme_CloseTimer(nvmeTimer);
                        nvmeTimer = NULL;

                        /* Install reset callback */
                        data->ac_ResetHandler.is_Node.ln_Pri = SD_PRI_DOS - 1;
                        data->ac_ResetHandler.is_Node.ln_Name = ((struct Node *)dev->dev_Controller)->ln_Name;
                        data->ac_ResetHandler.is_Code         = (VOID_FUNC)NVME_ResetHandler;
                        data->ac_ResetHandler.is_Data         = dev;
                        AddResetCallback(&data->ac_ResetHandler);
                        
                        HW_AddDriver(dev->dev_Controller, NVMEBase->busClass, attrs);
                    }
                    else
                    {
                        bug("[NVME:Controller] %s: ERROR - failed to query controller identity!\n", __func__);
                        data = NULL;
                    }
                    HIDD_PCIDriver_FreePCIMem(dev->dev_PCIDriverObject, buffer);
                }
                else
                {
                    D(bug ("[NVME:Controller] %s: ERROR - failed to create DMA buffer!\n", __func__);)
                    FreeMem(dev->dev_Queues[0]->cehooks, sizeof(_NVMEQUEUE_CE_HOOK) * 16);
                    FreeMem(dev->dev_Queues, sizeof(APTR) * (KrnGetCPUCount() + 1));
                    dev->dev_Queues = NULL;
                    // TODO: dispose the controller object
                    data = NULL;
                }
            }
            else
            {
                bug("[NVME:Controller] %s: ERROR - failed to create Admin Queue!\n", __func__);
                FreeMem(dev->dev_Queues, sizeof(APTR) * (KrnGetCPUCount() + 1));
                dev->dev_Queues = NULL;
                data = NULL;
            }
        }
        else
        {
            bug("[NVME:Controller] %s: ERROR - device data missing!\n", __func__);
            data = NULL;
        }
        if (data)
            AddTail(&NVMEBase->nvme_Controllers, &data->ac_Node);
    }

    D(bug ("[NVME:Controller] %s: returning 0x%p\n", __func__, nvmeController);)

    if (nvmeTimer)
        nvme_CloseTimer(nvmeTimer);

    return nvmeController;
}

VOID NVME__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    struct nvme_Controller *nvmeNode, *tmpNode;

    D(bug ("[NVME:Controller] %s(0x%p)\n", __func__, o);)

    ForeachNodeSafe (&NVMEBase->nvme_Controllers, nvmeNode, tmpNode)
    {
        if (nvmeNode->ac_Object == o)
        {
            D(bug ("[NVME:Controller] %s: Destroying Controller Entry @ %p\n", __func__, nvmeNode);)
            Remove(&nvmeNode->ac_Node);
        }
    }
}

void  NVME__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    struct nvme_Controller *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    HW_Switch(msg->attrID, idx)
    {
        case aoHW_Device:
            {
                if (data->ac_dev)
                    *msg->storage = (IPTR)data->ac_dev->dev_Object;
            }
            return;
        default:
            break;
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL NVME__Hidd_StorageController__RemoveBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_RemoveBus *Msg)
{
    D(bug ("[NVME:Controller] %s(0x%p)\n", __func__, o);)
   /*
     * Currently we don't support unloading NVME bus drivers.
     * This is a very-very big TODO.
     */
    return FALSE;
}

BOOL NVME__Hidd_StorageController__SetUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_SetUpBus *Msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;

    D(bug ("[NVME:Controller] %s(0x%p)\n", __func__, Msg->busObject);)

    /* Add the bus to the device and start service */
    return Hidd_NVMEBus_Start(Msg->busObject, NVMEBase);
}

void NVME__Hidd_StorageController__CleanUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_CleanUpBus *msg)
{
    D(bug ("[NVME:Controller] %s(0x%p)\n", __func__, o);)
    /* By default we have nothing to do here */
}
