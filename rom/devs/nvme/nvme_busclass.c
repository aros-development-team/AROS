/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/expansion.h>

#include <aros/atomic.h>
#include <libraries/expansion.h>

#include <utility/tagitem.h>

#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/storage.h>
#include <hidd/nvme.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "nvme_intern.h"
#include "nvme_hw.h"
#include "nvme_queue.h"
#include "nvme_queue_admin.h"

#define DIRQ(x)

/*
    NVME_IOIntCode:
        handle incomming completion event interrupts
        for bus IO queues.
*/
static AROS_INTH1(NVME_IOIntCode, struct nvme_queue *, nvmeq)
{
    AROS_INTFUNC_INIT

    D(bug ("[NVME:Bus] %s(0x%p)\n", __func__, nvmeq);)

    nvme_process_cq(nvmeq);

    D(bug ("[NVME:Bus] %s: finished\n", __func__);)

    return TRUE;

    AROS_INTFUNC_EXIT
}

OOP_Object *NVMEBus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    device_t dev = (device_t)GetTagData(aHidd_DriverData, 0, msg->attrList);
    IPTR unitCnt = GetTagData(aHidd_Bus_MaxUnits, 0, msg->attrList);

    char *devName = (char *)GetTagData(aHidd_StorageUnit_Model, 0, msg->attrList);
    char *devSer = (char *)GetTagData(aHidd_StorageUnit_Serial, 0, msg->attrList);
    char *devFW = (char *)GetTagData(aHidd_StorageUnit_Revision, 0, msg->attrList);


    D(bug ("[NVME:Bus] Root__New()\n");)

    if (!dev || !dev->dev_Queues[0] || unitCnt <= 0)
            return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct nvme_Bus *data = OOP_INST_DATA(cl, o);

        /* Cache device base pointer. Useful. */
        data->ab_Base = NVMEBase;
        D(bug ("[NVME:Bus] Root__New: NVMEBase @ %p\n", data->ab_Base);)

        data->ab_Dev = dev;

        if (devName)
        {
            data->ab_DevName = AllocVec(strlen(devName) + 1, MEMF_CLEAR);
            CopyMem(devName, data->ab_DevName, strlen(devName));
        }
        if (devSer)
        {
            data->ab_DevSer = AllocVec(strlen(devSer) + 1, MEMF_CLEAR);
            CopyMem(devSer, data->ab_DevSer, strlen(devSer));
        }
        if (devFW)
        {
            data->ab_DevFW = AllocVec(strlen(devFW) + 1, MEMF_CLEAR);
            CopyMem(devFW, data->ab_DevFW, strlen(devFW));
        }

        data->ab_UnitCnt = unitCnt;
        data->ab_Units = AllocMem(sizeof(OOP_Object *) * data->ab_UnitCnt, MEMF_CLEAR);
        D(bug ("[NVME:Bus] Root__New: Allocated pointers for %u unit(s) @ 0x%p\n", data->ab_UnitCnt, data->ab_Units);)
    }
    return o;
}

void NVMEBus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct nvme_Bus *data = OOP_INST_DATA(cl, o);
    D(bug ("[NVME:Bus] Root__Dispose(%p)\n", o);)

    if (data->ab_DevName)
    {
        FreeVec(data->ab_DevName);
    }
    if (data->ab_DevSer)
    {
        FreeVec(data->ab_DevSer);
    }
    if (data->ab_DevFW)
    {
        FreeVec(data->ab_DevFW);
    }

    OOP_DoSuperMethod(cl, o, msg);
}

/*
 * Here we take into account that the table can be either
 * terminated early, or have NULL entries.
 */
void NVMEBus__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    struct nvme_Bus *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Bus_Switch (msg->attrID, idx)
    {
    case aoHidd_Bus_MaxUnits:
        *msg->storage = data->ab_UnitCnt;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void NVMEBus__Hidd_StorageBus__EnumUnits(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageBus_EnumUnits *msg)
{
    struct nvme_Bus *data = OOP_INST_DATA(cl, o);
    int nn;

    D(bug ("[NVME:Bus] Hidd_StorageBus__EnumUnits()\n");)

    for (nn = 0; nn < data->ab_UnitCnt; nn++)
    {
        if (data->ab_Units[nn])
        {
            CALLHOOKPKT(msg->callback, data->ab_Units[nn], msg->hookMsg);
        }
    }
}

/*****************************************************************************************

    NAME
        moHidd_NVMEBus_Shutdown

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_NVMEBus_Shutdown *Msg);

        APTR HIDD_NVMEBus_Shutdown(void);

    LOCATION
        CLID_Hidd_NVMEBus

    FUNCTION
        Instantly shutdown all activity on the bus.

    INPUTS
        None

    RESULT
        None

    NOTES
        This method is called by nvme.device during system reset handler execution.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Default implementation disables interrupt using AltControl register.

*****************************************************************************************/

void NVMEBus__Hidd_NVMEBus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg *msg)
{
    D(bug ("[NVME:Bus] NVMEBus__Shutdown(%p)\n", o);)
}

/***************** Private nonvirtual methods follow *****************/

BOOL Hidd_NVMEBus_Start(OOP_Object *o, struct NVMEBase *NVMEBase)
{
    D(bug ("[NVME:Bus] NVMEBus_Start(%p)\n", o);)
    struct nvme_Bus *data = OOP_INST_DATA(NVMEBase->busClass, o);
    APTR buffer = HIDD_PCIDriver_AllocPCIMem(data->ab_Dev->dev_PCIDriverObject, 8192);
    UQUAD lbaStart, lbaEnd;
    struct ExpansionBase *ExpansionBase;
    struct nvme_command c;
    struct completionevent_handler busehandle;
    int nn;

    if (buffer)
    {
        IPTR PCIIntLine = 0, AdminIntLine;
        struct TagItem vectreqs[] = 
        {
            { tHidd_PCIVector_Min,      1                       },
            { tHidd_PCIVector_Max,      KrnGetCPUCount()        },
            { TAG_DONE,                 0                       } 
        };
        int depth, hwqcnt = 1;

        ExpansionBase = (struct ExpansionBase *)TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);

        D(bug ("[NVME:Bus] NVMEBus_Start: buffer @ 0x%p, ExpansionBase @ 0x%p\n", buffer, ExpansionBase);)

        busehandle.ceh_Task = FindTask(NULL);
        busehandle.ceh_SigSet = SIGF_SINGLE;
        OOP_GetAttr(data->ab_Dev->dev_Object, aHidd_PCIDevice_INTLine, &PCIIntLine);
        AdminIntLine = PCIIntLine;

        DIRQ(bug ("[NVME:Bus] NVMEBus_Start: Initial Admin IRQ = %u\n", AdminIntLine);)

        memset(&c, 0, sizeof(c));
        c.features.op.opcode = nvme_admin_set_features;
        c.features.fid = AROS_LONG2LE(NVME_FEAT_NUM_QUEUES);
        c.features.dword11 = AROS_LONG2LE(vectreqs[1].ti_Data | (vectreqs[1].ti_Data << 16));
        D(bug ("[NVME:Bus] NVMEBus_Start: sending nvme_admin_set_features(NVME_FEAT_NUM_QUEUES, %u)\n", vectreqs[1].ti_Data);)
        nvme_submit_admincmd(data->ab_Dev, &c, &busehandle);
        Wait(busehandle.ceh_SigSet);
        if (!busehandle.ceh_Status)
        {
            int featqueues = MIN((busehandle.ceh_Result & 0xffff), (busehandle.ceh_Result >> 16)) + 1;
            if (featqueues < vectreqs[1].ti_Data)
                vectreqs[1].ti_Data = featqueues;
        }

        DIRQ(bug ("[NVME:Bus] NVMEBus_Start: required queues = %u\n", vectreqs[1].ti_Data);)

        Disable();
        DIRQ(bug ("[NVME:Bus] NVMEBus_Start: Removing Admin Int Server...\n");)
        // Remove the device Admin Interrupt handler
        HIDD_PCIDriver_RemoveInterrupt(data->ab_Dev->dev_PCIDriverObject, data->ab_Dev->dev_Object, &data->ab_Dev->dev_Queues[0]->q_IntHandler);

        DIRQ(bug ("[NVME:Bus] NVMEBus_Start: Attempting to Obtain PCI Device MSI Vectors ...\n");)
        if (HIDD_PCIDevice_ObtainVectors(data->ab_Dev->dev_Object, vectreqs))
        {
            struct TagItem vecAttribs[] =
            {
                {   tHidd_PCIVector_Int,    (IPTR)-1        },
                {   TAG_DONE,               0               }
            };
            DIRQ(bug ("[NVME:Bus] NVMEBus_Start: Obtained MSI Vectors!\n");)
            //Switch The Admin Queue IRQ;
            HIDD_PCIDevice_GetVectorAttribs(data->ab_Dev->dev_Object, 0, vecAttribs);
            if (vecAttribs[0].ti_Data != (IPTR)-1)
                AdminIntLine = vecAttribs[0].ti_Data;
            //hwqcnt = ;
        }
        DIRQ(bug ("[NVME:Bus] NVMEBus_Start: Re-Adding Admin Int Server ...\n");)
        // Add the hardware IRQ Admin Interrupt handler
        AddIntServer(INTB_KERNEL + AdminIntLine,
            &data->ab_Dev->dev_Queues[0]->q_IntHandler);
        Enable();

        DIRQ(bug ("[NVME:Bus] NVMEBus_Start: Ending Admin IRQ = %u\n", AdminIntLine);)

        /* Prepare the IO queuue(s) */

        depth = MIN(NVME_CAP_MQES(data->ab_Dev->dev_nvmeregbase->cap) + 1, 1024);

        for (nn = 0; nn < hwqcnt; nn++)
        {
            data->ab_Dev->dev_Queues[nn + 1] = nvme_alloc_queue(data->ab_Dev, nn + 1, depth, nn);
            D(bug ("[NVME:Bus] NVMEBus_Start:  # IO queue @ 0x%p (depth = %u)\n", data->ab_Dev->dev_Queues[nn + 1], depth);)
            if (data->ab_Dev->dev_Queues[nn + 1])
            {
                struct TagItem vecAttribs[] =
                {
                    {   tHidd_PCIVector_Int,    (IPTR)-1        },
                    {   tHidd_PCIVector_Native, (IPTR)-1        },
                    {   TAG_DONE,               0               }
                };
                int flags;

                HIDD_PCIDevice_GetVectorAttribs(data->ab_Dev->dev_Object, data->ab_Dev->dev_Queues[nn + 1]->cq_vector, vecAttribs);                

                if ((vecAttribs[0].ti_Data != (IPTR)-1) && (vecAttribs[1].ti_Data != (IPTR)-1))
                {
                    D(bug ("[NVME:Bus] NVMEBus_Start:     IRQ #%u\n", vecAttribs[0].ti_Data);)

                    data->ab_Dev->dev_Queues[nn + 1]->cehooks = AllocMem(sizeof(_NVMEQUEUE_CE_HOOK) * 16, MEMF_CLEAR);
                    data->ab_Dev->dev_Queues[nn + 1]->cehandlers = AllocMem(sizeof(struct completionevent_handler *) * 16, MEMF_CLEAR);

                    D(bug ("[NVME:Bus] NVMEBus_Start:     hooks @ 0x%p, handlers @ 0x%p\n", data->ab_Dev->dev_Queues[nn + 1]->cehooks, data->ab_Dev->dev_Queues[nn + 1]->cehandlers);)
                    
                    /* completion queue needs to be set before the submission queue */
                    flags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;

                    memset(&c, 0, sizeof(c));
                    c.create_cq.op.opcode = nvme_admin_create_cq;
                    c.create_cq.prp1 = (IPTR)data->ab_Dev->dev_Queues[nn + 1]->cqba; // Needs to be LE
                    c.create_cq.cqid = AROS_WORD2LE(nn + 1);
                    c.create_cq.qsize = AROS_WORD2LE(data->ab_Dev->dev_Queues[nn + 1]->q_depth - 1);
                    c.create_cq.cq_flags = AROS_WORD2LE(flags);
                    c.create_cq.irq_vector = AROS_WORD2LE(vecAttribs[1].ti_Data);

                    nvme_submit_admincmd(data->ab_Dev, &c, &busehandle);
                    Wait(busehandle.ceh_SigSet);
                    if (!busehandle.ceh_Status)
                    {
                        flags = NVME_QUEUE_PHYS_CONTIG | NVME_SQ_PRIO_MEDIUM;

                        memset(&c, 0, sizeof(c));
                        c.create_sq.op.opcode = nvme_admin_create_sq;
                        c.create_sq.prp1 = (IPTR)data->ab_Dev->dev_Queues[nn + 1]->sqba; // Needs to be LE
                        c.create_sq.sqid = AROS_WORD2LE(nn + 1);
                        c.create_sq.qsize = AROS_WORD2LE(data->ab_Dev->dev_Queues[nn + 1]->q_depth - 1);
                        c.create_sq.sq_flags = AROS_WORD2LE(flags);
                        c.create_sq.cqid = AROS_WORD2LE(nn + 1);

                        nvme_submit_admincmd(data->ab_Dev, &c, &busehandle);
                        Wait(busehandle.ceh_SigSet);
                        if (!busehandle.ceh_Status)
                        {
                            data->ab_Dev->dev_Queues[nn + 1]->q_IntHandler.is_Node.ln_Name = "NVME IO Interrupt";
                            data->ab_Dev->dev_Queues[nn + 1]->q_IntHandler.is_Node.ln_Pri = 0;
                            data->ab_Dev->dev_Queues[nn + 1]->q_IntHandler.is_Code = (VOID_FUNC) NVME_IOIntCode;
                            data->ab_Dev->dev_Queues[nn + 1]->q_IntHandler.is_Data = data->ab_Dev->dev_Queues[nn + 1];
                            AddIntServer(INTB_KERNEL + vecAttribs[0].ti_Data,
                                &data->ab_Dev->dev_Queues[nn + 1]->q_IntHandler);
                            data->ab_Dev->queuecnt++;
                        }
                        else
                        {
                            bug ("[NVME:Bus] NVMEBus_Start: ERROR - failed to register IO submission queue (status=%u)\n", busehandle.ceh_Status);
                        }
                    }
                    else
                    {
                        bug ("[NVME:Bus] NVMEBus_Start: ERROR - failed to register IO completion queue (status=%u)\n", busehandle.ceh_Status);
                    }
                }
                else
                {
                    bug ("[NVME:Bus] NVMEBus_Start: ERROR - failed to obtain necessary vector attribs\n");
                }
            }
            else
            {
                bug ("[NVME:Bus] NVMEBus_Start: ERROR - failed to allocate IO queue\n");
            }
        }

        /* Attach detected Units */

        for (nn = 0; nn < data->ab_UnitCnt; nn++)
        {
            struct nvme_id_ns *id_ns = (struct nvme_id_ns *)buffer;
            struct TagItem attrs[] =
            {
                    {aHidd_Name,                        (IPTR)"nvme.device"     },
                    {aHidd_DriverData,                  (IPTR)data->ab_Dev      },
                    {aHidd_StorageUnit_Number,          nn                      },
                    {aHidd_StorageUnit_Model,           (IPTR)data->ab_DevName  },
                    {aHidd_StorageUnit_Serial,          (IPTR)data->ab_DevSer   },
                    {aHidd_StorageUnit_Revision,        (IPTR)data->ab_DevFW    },
                    {TAG_DONE,                          0                       }
            };

            memset(buffer, 0, 8192);
            memset(&c, 0, sizeof(c));
            c.identify.op.opcode = nvme_admin_identify;
            c.identify.prp1 = (UQUAD)(IPTR)buffer;
            c.identify.nsid = AROS_LONG2LE(nn + 1);
            c.identify.cns = 0;

            D(bug ("[NVME:Bus] NVMEBus_Start: ns#%u sending nvme_admin_identify\n", nn + 1);)
            nvme_submit_admincmd(data->ab_Dev, &c, &busehandle);
            Wait(busehandle.ceh_SigSet);

            if ((!busehandle.ceh_Status) && (id_ns->ncap != 0))
            {
                int lbaf = id_ns->flbas & 0xf;
                lbaStart = 0;

                D(bug ("[NVME:Bus] NVMEBus_Start: ns#%u     ncap = %p\n", nn + 1, id_ns->ncap);)
            
                struct nvme_lba_range_type *rt = (struct nvme_lba_range_type *)(IPTR)buffer + 4096;

                D(bug ("[NVME:Bus] NVMEBus_Start: ns#%u lba_range_type buffer @ 0x%p\n", nn + 1, rt);)

                memset(&c, 0, sizeof(c));
                c.features.op.opcode = nvme_admin_get_features;
                c.features.prp1 = (UQUAD)(IPTR)rt;
                c.features.fid = AROS_LONG2LE(NVME_FEAT_LBA_RANGE);
                c.features.nsid = AROS_LONG2LE(nn + 1);

                D(bug ("[NVME:Bus] NVMEBus_Start: ns#%u sending nvme_admin_get_features\n", nn + 1);)
                nvme_submit_admincmd(data->ab_Dev, &c, &busehandle);
                Wait(busehandle.ceh_SigSet);

                if (!busehandle.ceh_Status)
                {
                    D(
                        bug ("[NVME:Bus] NVMEBus_Start: ns#%u range type info:\n", nn + 1);
                        bug ("[NVME:Bus] NVMEBus_Start: ns#%u           type %02x\n", nn + 1, rt->type);
                        bug ("[NVME:Bus] NVMEBus_Start: ns#%u           attribs %02x\n", nn + 1, rt->attributes);
                        bug ("[NVME:Bus] NVMEBus_Start: ns#%u           slba %08x%08x\n", nn + 1, (rt->slba >> 32), rt->slba & 0xFFFFFFFF);
                        bug ("[NVME:Bus] NVMEBus_Start: ns#%u           nlb %08x%08x\n", nn + 1, (rt->nlb >> 32), rt->nlb & 0xFFFFFFFF);
                    )

                    if (!(rt->attributes & NVME_LBART_ATTRIB_HIDE))
                    {
                        lbaStart = rt->slba;
                        lbaEnd =  rt->slba + rt->nlb;
                    }
                    else
                    {
                        D(bug ("[NVME:Bus] NVMEBus_Start:      Skipping hidden namespace\n");)
                        lbaEnd = 0;
                    }
                }
                else
                    lbaEnd = id_ns->nsze << (id_ns->lbaf[lbaf].ds - 9);

                if (lbaEnd)
                {
                    if ((data->ab_Units[nn] = OOP_NewObject(NVMEBase->unitClass, NULL, attrs)) != NULL)
                    {
                        struct nvme_Unit *unit = OOP_INST_DATA(NVMEBase->unitClass, data->ab_Units[nn]);
                        const ULONG IdDOS = AROS_MAKE_ID('D','O','S','\001');
                        IPTR pp[24];

                        if (ExpansionBase)
                        {
                            const TEXT dosdevstem[3] = "HD";
                            struct TagItem NVMEIDTags[] = 
                            {
                                {tHidd_Storage_IDStem,  (IPTR)dosdevstem        },
                                {TAG_DONE,              0                       }  
                            };
                            struct DeviceNode *devnode;

                            D(bug ("[NVME:Bus] NVMEBus_Start:      Sector Size = %u\n", 1 << id_ns->lbaf[lbaf].ds);)
                            D(bug ("[NVME:Bus] NVMEBus_Start:      # of Sectors = %u\n", id_ns->nsze << (id_ns->lbaf[lbaf].ds - 9));)

                            unit->au_SecShift = id_ns->lbaf[lbaf].ds;
                            unit->au_SecCnt = id_ns->nsze << (unit->au_SecShift - 9);
                            unit->au_Low = lbaStart;
                            unit->au_High = lbaEnd;
                            unit->au_Bus = data;

                            data->ab_IDNode = HIDD_Storage_AllocateID(NVMEBase->storageRoot, NVMEIDTags);

                            pp[0] 		    = (IPTR)data->ab_IDNode->ln_Name;
                            pp[1]		    = (IPTR)MOD_NAME_STRING;
                            pp[2]		    = unit->au_UnitNum;
                            pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
                            pp[DE_SIZEBLOCK    + 4] = 1 << unit->au_SecShift;
                            pp[DE_NUMHEADS     + 4] = 1;
                            pp[DE_SECSPERBLOCK + 4] = 1;
                            pp[DE_BLKSPERTRACK + 4] = unit->au_SecCnt;
                            pp[DE_RESERVEDBLKS + 4] = 2;
                            pp[DE_LOWCYL       + 4] = unit->au_Low;
                            pp[DE_HIGHCYL      + 4] = unit->au_High;
                            pp[DE_NUMBUFFERS   + 4] = 10;
                            pp[DE_BUFMEMTYPE   + 4] = MEMF_PUBLIC | MEMF_31BIT;
                            pp[DE_MAXTRANSFER  + 4] = 0x00200000;
                            pp[DE_MASK         + 4] = 0x7FFFFFFE;
                            pp[DE_BOOTPRI      + 4] = 0;
                            pp[DE_DOSTYPE      + 4] = IdDOS;
                            pp[DE_CONTROL      + 4] = 0;
                            pp[DE_BOOTBLOCKS   + 4] = 2;

                            devnode = MakeDosNode(pp);
                            if (devnode)
                            {
                                D(bug("[NVME:Bus] NVMEBus_Start: DeviceNode @ 0x%p\n", devnode));
                                D(bug("[NVME:Bus] NVMEBus_Start:'%b', type=0x%08lx with StartCyl=%d, EndCyl=%d .. ",
                                      devnode->dn_Name, pp[DE_DOSTYPE + 4], pp[DE_LOWCYL + 4], pp[DE_HIGHCYL + 4]));

                                AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, NULL);
                                D(bug("done\n"));
                            }
                        }
                    }
                    else
                    {
                        bug("[NVME:Bus] NVMEBus_Start: Failed to create ns#%u unit\n", nn + 1);
                    }
                }
            }
        }

        CloseLibrary((struct Library *)ExpansionBase);

        HIDD_PCIDriver_FreePCIMem(data->ab_Dev->dev_PCIDriverObject, buffer);
        return TRUE;
    }
    return FALSE;
}


AROS_UFH3(BOOL, Hidd_NVMEBus_Open,
          AROS_UFHA(struct Hook *, h, A0),
          AROS_UFHA(OOP_Object *, obj, A2),
          AROS_UFHA(IPTR, reqUnit, A1))
{
    AROS_USERFUNC_INIT

    D(bug ("[NVME:Bus] Hidd_NVMEBus_Open(%p)\n", obj);)

    struct IORequest *req = h->h_Data;
    struct NVMEBase *NVMEBase = (struct NVMEBase *)req->io_Device;
    struct nvme_Bus *b = (struct nvme_Bus *)OOP_INST_DATA(NVMEBase->busClass, obj);
    int nn = reqUnit & ((1 << 12) - 1);

    if (((reqUnit >> 12) == b->ab_Dev->dev_HostID) && (nn < b->ab_UnitCnt) && b->ab_Units[nn])
    {
        struct nvme_Unit *unit = (struct nvme_Unit *)OOP_INST_DATA(NVMEBase->unitClass, b->ab_Units[nn]);

        /* Got the unit */
        req->io_Unit  = &unit->au_Unit;
        req->io_Error = 0;

        AROS_ATOMIC_INC(unit->au_Unit.unit_OpenCnt);
        return TRUE;
    }
    return FALSE;

    AROS_USERFUNC_EXIT
}
