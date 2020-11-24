/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/utility.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>
#include <hidd/nvme.h>
#include <oop/oop.h>

#include "nvme_intern.h"
#include "nvme_hw.h"
#include "nvme_queue.h"
#include "nvme_queue_admin.h"

static const char *str_devicename = "nvme.device";

/*
    NVME_IOIntCode:
        handle incomming completion event interrupts
        for a units IO queue.
*/
static AROS_INTH1(NVME_IOIntCode, struct nvme_queue *, nvmeq)
{
    AROS_INTFUNC_INIT

    D(bug ("[NVME:Unit] %s(0x%p)\n", __func__, nvmeq);)

    nvme_process_cq(nvmeq);
    
    return FALSE;

    AROS_INTFUNC_EXIT
}

/* support functions */
static void nvme_strcpy(const UBYTE *str1, UBYTE *str2, ULONG size)
{
    register int i = size;

    while (i--)
    {
        if (str1[i] < ' ')
            str2[i] = '\0';
        else
            str2[i] = str1[i];
    }
}

/* Unit class methods */
OOP_Object *NVMEUnit__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    int unitnsno = (int)GetTagData(aHidd_StorageUnit_Number, 0, msg->attrList);
    device_t dev = (device_t)GetTagData(aHidd_DriverData, 0, msg->attrList);

    D(bug ("[NVME:Unit] Root__New()\n");)

    if (!dev)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct nvme_Unit *unit = OOP_INST_DATA(cl, o);
        D(bug ("[NVME:Unit] Root__New: Unit Obj @ %p\n", o);)

        InitSemaphore(&unit->au_Lock);
        NEWLIST(&unit->au_IOs);
        unit->au_UnitNum = (dev->dev_HostID << 12) | unitnsno;
        
        char *DevName = (char *)GetTagData(aHidd_StorageUnit_Model, 0, msg->attrList);
        char *DevSer = (char *)GetTagData(aHidd_StorageUnit_Serial, 0, msg->attrList);
        char *DevFW = (char *)GetTagData(aHidd_StorageUnit_Revision, 0, msg->attrList);

        if (DevName)
        {
            nvme_strcpy(DevName, unit->au_Model, 40);
            D(bug ("[NVME:Unit] Root__New: Model    %s\n", unit->au_Model);)
        }
        if (DevSer)
        {
            nvme_strcpy(DevSer, unit->au_SerialNumber, 20);
            D(bug ("[NVME:Unit] Root__New: Serial   %s\n", unit->au_SerialNumber);)
        }
        if (DevFW)
        {
            nvme_strcpy(DevFW, unit->au_FirmwareRev, 8);
            D(bug ("[NVME:Unit] Root__New: FW Revis %s\n", unit->au_FirmwareRev);)
        }

#if !defined(MIN)
# define MIN(a,b)       (a < b) ? a : b
#endif
        int depth = MIN(NVME_CAP_MQES(dev->dev_nvmeregbase->cap) + 1, 1024);
        // TODO: Allocate an MSI interrupt!
        int vector = 80 + unitnsno;

        unit->au_IOQueue = nvme_alloc_queue(dev, unitnsno + 1, depth, vector);
        D(bug ("[NVME:Unit] Root__New:     IO queue @ 0x%p (IRQ#%u, depth = %u)\n", unit->au_IOQueue, vector, depth);)
        if (unit->au_IOQueue)
        {
            struct nvme_command c;
            struct completionevent_handler adminehandle;
            int flags;

            unit->au_IOQueue->cehooks = AllocMem(sizeof(_NVMEQUEUE_CE_HOOK) * 16, MEMF_CLEAR);
            unit->au_IOQueue->cehandlers = AllocMem(sizeof(struct completionevent_handler *) * 16, MEMF_CLEAR);

            adminehandle.ceh_Task = FindTask(NULL);
            adminehandle.ceh_SigSet = SIGF_SINGLE;

            /* completion queue needs to be set before the submission queue */
            flags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;

            memset(&c, 0, sizeof(c));
            c.create_cq.op.opcode = nvme_admin_create_cq;
            c.create_cq.prp1 = (IPTR)unit->au_IOQueue->cqba; // Needs to be LE
            c.create_cq.cqid = AROS_WORD2LE(unitnsno + 1);
            c.create_cq.qsize = AROS_WORD2LE(unit->au_IOQueue->q_depth - 1);
            c.create_cq.cq_flags = AROS_WORD2LE(flags);
            c.create_cq.irq_vector = AROS_WORD2LE(unit->au_IOQueue->cq_vector);

            nvme_submit_admincmd(dev, &c, &adminehandle);
            Wait(adminehandle.ceh_SigSet);
            if (!adminehandle.ceh_Status)
            {
                flags = NVME_QUEUE_PHYS_CONTIG | NVME_SQ_PRIO_MEDIUM;

                memset(&c, 0, sizeof(c));
                c.create_sq.op.opcode = nvme_admin_create_sq;
                c.create_sq.prp1 = (IPTR)unit->au_IOQueue->sqba; // Needs to be LE
                c.create_sq.sqid = AROS_WORD2LE(unitnsno + 1);
                c.create_sq.qsize = AROS_WORD2LE(unit->au_IOQueue->q_depth - 1);
                c.create_sq.sq_flags = AROS_WORD2LE(flags);
                c.create_sq.cqid = AROS_WORD2LE(unitnsno + 1);

                nvme_submit_admincmd(dev, &c, &adminehandle);
                Wait(adminehandle.ceh_SigSet);
                if (!adminehandle.ceh_Status)
                {
                    unit->au_IOIntHandler.is_Node.ln_Name = "NVME IO Interrupt";
                    unit->au_IOIntHandler.is_Node.ln_Pri = 5;
                    unit->au_IOIntHandler.is_Code = (VOID_FUNC) NVME_IOIntCode;
                    unit->au_IOIntHandler.is_Data = unit->au_IOQueue;
                    AddIntServer(INTB_KERNEL + vector,
                        &unit->au_IOIntHandler);
                }
                else
                {
                    // TODO: Tear down -:
                    // completion queue we just created..
                    // ioqueue
                    // coerce dispose
                    bug ("[NVME:Unit] Root__New: ERROR - failed to register IO submission queue (status=%u)\n", adminehandle.ceh_Status);
                }
            }
            else
            {
                // TODO: Tear down -:
                // ioqueue
                // coerce dispose
                bug ("[NVME:Unit] Root__New: ERROR - failed to register IO completion queue (status=%u)\n", adminehandle.ceh_Status);
            }
        }
        else
        {
            // TODO: Tear down -:
            // coerce dispose
        }
    }
    return o;
}

void NVMEUnit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug ("[NVME:Unit] Root__Dispose(%p)\n", o);)

    OOP_DoSuperMethod(cl, o, msg);
}

void NVMEUnit__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    struct nvme_Unit *unit = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_StorageUnit_Switch (msg->attrID, idx)
    {
    case aoHidd_StorageUnit_Device:
        *msg->storage = (IPTR)str_devicename;
        return;

    case aoHidd_StorageUnit_Number:
        *msg->storage = unit->au_UnitNum;
        return;

    case aoHidd_StorageUnit_Type:
        *msg->storage = vHidd_StorageUnit_Type_SolidStateDisk;
        return;

    case aoHidd_StorageUnit_Model:
        *msg->storage = (IPTR)unit->au_Model;
        return;

    case aoHidd_StorageUnit_Revision:
        *msg->storage = (IPTR)unit->au_FirmwareRev;
        return;

    case aoHidd_StorageUnit_Serial:
        *msg->storage = (IPTR)unit->au_SerialNumber;
        return;

    case aoHidd_StorageUnit_Removable:
        *msg->storage = (IPTR)FALSE;
        return;
    }

#if (0)
    Hidd_NVMEUnit_Switch (msg->attrID, idx)
    {
    case aoHidd_NVMEUnit_Features:
        *msg->storage = (IPTR)unit->au_features;
        return;
    }
#endif

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
