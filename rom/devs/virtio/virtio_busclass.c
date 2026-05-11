/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#define __NOLIBBASE__

#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/expansion.h>

#include <aros/atomic.h>
#include <libraries/expansion.h>
#include <utility/tagitem.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <dos/bptr.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/errors.h>

#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/storage.h>
#include <hidd/virtio.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "virtio_intern.h"
#include "virtio_hw.h"
#include "virtio_queue.h"

extern const char GM_UNIQUENAME(LibName)[];

static AROS_INTH1(VIRTIO_IOIntCode, struct virtio_queue *, vq)
{
    AROS_INTFUNC_INIT
    /* Acknowledge ISR (modern: read clears) */
    if (vq->dev && vq->dev->dev_ISR) {
        (void) *vq->dev->dev_ISR;
    }
    Signal(vq->q_IOTask, SIGBREAKF_CTRL_F);
    return TRUE;
    AROS_INTFUNC_EXIT
}

static void virtio_iotask(struct virtio_queue *vq)
{
    SetSignal(0, SIGBREAKF_CTRL_F);
    for (;;) {
        Wait(SIGBREAKF_CTRL_F);
        virtio_process_used(vq);
    }
}

OOP_Object *VirtIOBus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct VirtIOBase *VirtIOBase = cl->UserData;
    device_t dev = (device_t)GetTagData(aHidd_DriverData, 0, msg->attrList);
    IPTR maxUnits = GetTagData(aHidd_Bus_MaxUnits, 0, msg->attrList);
    char *devName = (char *)GetTagData(aHidd_StorageUnit_Model, 0, msg->attrList);
    char *devSer  = (char *)GetTagData(aHidd_StorageUnit_Serial, 0, msg->attrList);
    char *devFW   = (char *)GetTagData(aHidd_StorageUnit_Revision, 0, msg->attrList);

    if (!dev || !dev->dev_VQ || maxUnits <= 0)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o) {
        struct virtio_Bus *data = OOP_INST_DATA(cl, o);

        data->ab_Base = VirtIOBase;
        data->ab_Dev  = dev;

        if (devName) {
            data->ab_DevName = AllocVec(strlen(devName) + 1, MEMF_CLEAR);
            if (data->ab_DevName) CopyMem(devName, data->ab_DevName, strlen(devName));
        }
        if (devSer) {
            data->ab_DevSer = AllocVec(strlen(devSer) + 1, MEMF_CLEAR);
            if (data->ab_DevSer) CopyMem(devSer, data->ab_DevSer, strlen(devSer));
        }
        if (devFW) {
            data->ab_DevFW = AllocVec(strlen(devFW) + 1, MEMF_CLEAR);
            if (data->ab_DevFW) CopyMem(devFW, data->ab_DevFW, strlen(devFW));
        }

        data->ab_UnitMax = maxUnits;
        data->ab_Units = AllocMem(sizeof(OOP_Object *) * maxUnits, MEMF_CLEAR);
    }
    return o;
}

void VirtIOBus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct virtio_Bus *data = OOP_INST_DATA(cl, o);

    if (data->ab_DevName) FreeVec(data->ab_DevName);
    if (data->ab_DevSer)  FreeVec(data->ab_DevSer);
    if (data->ab_DevFW)   FreeVec(data->ab_DevFW);

    OOP_DoSuperMethod(cl, o, msg);
}

void VirtIOBus__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct VirtIOBase *VirtIOBase = (struct VirtIOBase *)cl->UserData;
    struct virtio_Bus *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Bus_Switch(msg->attrID, idx) {
    case aoHidd_Bus_MaxUnits:
        *msg->storage = data->ab_UnitMax;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void VirtIOBus__Hidd_StorageBus__EnumUnits(OOP_Class *cl, OOP_Object *o,
                                           struct pHidd_StorageBus_EnumUnits *msg)
{
    struct virtio_Bus *data = OOP_INST_DATA(cl, o);
    int nn;
    for (nn = 0; nn < data->ab_UnitMax; nn++) {
        if (data->ab_Units[nn]) {
            CALLHOOKPKT(msg->callback, data->ab_Units[nn], msg->hookMsg);
        }
    }
}

void VirtIOBus__Hidd_VirtIOBus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg *msg)
{
    /* Best effort - put device back to status 0. */
    struct virtio_Bus *data = OOP_INST_DATA(cl, o);
    if (data && data->ab_Dev && data->ab_Dev->dev_CommonCfg)
        data->ab_Dev->dev_CommonCfg->device_status = 0;
}

BOOL Hidd_VirtIOBus_Start(OOP_Object *o, struct VirtIOBase *VirtIOBase)
{
    struct virtio_Bus *data = OOP_INST_DATA(VirtIOBase->busClass, o);
    device_t dev = data->ab_Dev;
    struct ExpansionBase *ExpansionBase;
    UQUAD secCnt = dev->dev_Capacity;       /* in 512-byte sectors */
    ULONG secSize = dev->dev_BlkSize ? dev->dev_BlkSize : VIRTIO_BLK_SECTOR_SIZE;

    /* Sector count in unit terms (logical-block sized): */
    if (secSize > VIRTIO_BLK_SECTOR_SIZE) {
        secCnt = secCnt / (secSize / VIRTIO_BLK_SECTOR_SIZE);
    }

    if (secCnt == 0) {
        bug("[VIRTIO:Bus] %s: zero capacity, refusing to attach unit\n", __func__);
        return FALSE;
    }

    /* Spin up IO completion task */
    dev->dev_VQ->q_IOTask = NewCreateTask(TASKTAG_NAME, "VirtIO IO task",
                                          TASKTAG_PC,   virtio_iotask,
                                          TASKTAG_PRI,  21,
                                          TASKTAG_ARG1, dev->dev_VQ,
                                          TAG_END);
    if (!dev->dev_VQ->q_IOTask) {
        bug("[VIRTIO:Bus] %s: failed to spawn IO task\n", __func__);
        return FALSE;
    }

    /* Hook the device's PCI INTx line to the IO interrupt handler. */
    dev->dev_VQ->q_IntHandler.is_Node.ln_Name = "VirtIO Block IRQ";
    dev->dev_VQ->q_IntHandler.is_Node.ln_Pri  = 0;
    dev->dev_VQ->q_IntHandler.is_Code = (VOID_FUNC)VIRTIO_IOIntCode;
    dev->dev_VQ->q_IntHandler.is_Data = dev->dev_VQ;
    if (!HIDD_PCIDriver_AddInterrupt(dev->dev_PCIDriverObject, dev->dev_Object,
                                     &dev->dev_VQ->q_IntHandler)) {
        bug("[VIRTIO:Bus] %s: failed to register PCI interrupt handler\n", __func__);
        return FALSE;
    }

    /* Mark the device as DRIVER_OK now we have everything wired up. */
    if (!virtio_hw_driver_ok(dev)) {
        bug("[VIRTIO:Bus] %s: device transition to DRIVER_OK failed\n", __func__);
        return FALSE;
    }

    /* Build & register the unit + its DOS bootnode. */
    ExpansionBase = (struct ExpansionBase *)TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);
    if (!ExpansionBase)
        return FALSE;

    {
        struct TagItem attrs[] = {
            {aHidd_Name,                        (IPTR)GM_UNIQUENAME(LibName)    },
            {aHidd_DriverData,                  (IPTR)dev                       },
            {aHidd_StorageUnit_Number,          0                               },
            {aHidd_StorageUnit_Model,           (IPTR)data->ab_DevName          },
            {aHidd_StorageUnit_Serial,          (IPTR)data->ab_DevSer           },
            {aHidd_StorageUnit_Revision,        (IPTR)data->ab_DevFW            },
            {TAG_DONE,                          0                               }
        };
        const TEXT dosdevstem[3] = "HD";
        struct TagItem VIDTags[] = {
            {tHidd_Storage_IDStem,  (IPTR)dosdevstem},
            {TAG_DONE,              0               }
        };
        struct DeviceNode *devnode;
        const ULONG IdDOS = AROS_MAKE_ID('D','O','S','\001');
        IPTR pp[4 + DE_BOOTBLOCKS + 1];
        struct virtio_Unit *unit;
        ULONG sshift = 0;
        ULONG sz;

        for (sz = secSize; sz > 1; sz >>= 1) sshift++;

        if ((data->ab_Units[0] = OOP_NewObject(VirtIOBase->unitClass, NULL, attrs)) == NULL) {
            bug("[VIRTIO:Bus] %s: failed to create unit object\n", __func__);
            CloseLibrary((struct Library *)ExpansionBase);
            return FALSE;
        }

        unit = OOP_INST_DATA(VirtIOBase->unitClass, data->ab_Units[0]);
        unit->au_Bus      = data;
        unit->au_SecShift = sshift;
        unit->au_SecCnt   = secCnt;
        unit->au_Low      = 0;
        unit->au_High     = secCnt - 1;

        /* CHS - synthesise reasonable values from capacity if device didn't tell us */
        if (dev->dev_Heads && dev->dev_Sectors) {
            unit->nu_Heads = dev->dev_Heads;
        } else {
            unit->nu_Heads = 16;
        }
        if (unit->nu_Heads == 0) unit->nu_Heads = 1;
        unit->nu_Cyl = (ULONG)(secCnt / (unit->nu_Heads * 63));
        if (unit->nu_Cyl == 0) unit->nu_Cyl = 1;

        data->ab_IDNode = HIDD_Storage_AllocateID(VirtIOBase->storageRoot, VIDTags);

        pp[0]                   = (IPTR)data->ab_IDNode->ln_Name;
        pp[1]                   = (IPTR)MOD_NAME_STRING;
        pp[2]                   = unit->au_UnitNum;
        pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
        pp[DE_SIZEBLOCK    + 4] = 1 << (unit->au_SecShift - 2);
        pp[DE_NUMHEADS     + 4] = unit->nu_Heads;
        pp[DE_SECSPERBLOCK + 4] = 1;
        pp[DE_BLKSPERTRACK + 4] = 63;
        pp[DE_RESERVEDBLKS + 4] = 2;
        pp[DE_LOWCYL       + 4] = (ULONG)unit->au_Low;
        pp[DE_HIGHCYL      + 4] = unit->nu_Cyl - 1;
        pp[DE_NUMBUFFERS   + 4] = 10;
        pp[DE_BUFMEMTYPE   + 4] = MEMF_PUBLIC;
        pp[DE_MAXTRANSFER  + 4] = 0x00200000;       /* 2MB max per request */
        pp[DE_MASK         + 4] = 0x7FFFFFFF;
        pp[DE_BOOTPRI      + 4] = 0;
        pp[DE_DOSTYPE      + 4] = IdDOS;
        pp[DE_CONTROL      + 4] = 0;
        pp[DE_BOOTBLOCKS   + 4] = 2;

        devnode = MakeDosNode(pp);
        if (devnode) {
            AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, NULL);
        }

        data->ab_UnitCnt = 1;
    }

    CloseLibrary((struct Library *)ExpansionBase);
    return TRUE;
}

AROS_UFH3(BOOL, Hidd_VirtIOBus_Open,
          AROS_UFHA(struct Hook *, h, A0),
          AROS_UFHA(OOP_Object *, obj, A2),
          AROS_UFHA(IPTR, reqUnit, A1))
{
    AROS_USERFUNC_INIT

    struct IORequest *req = h->h_Data;
    struct VirtIOBase *VirtIOBase = (struct VirtIOBase *)req->io_Device;
    struct virtio_Bus *b = (struct virtio_Bus *)OOP_INST_DATA(VirtIOBase->busClass, obj);

    if ((reqUnit >> 12) == b->ab_Dev->dev_HostID) {
        int nn = reqUnit & ((1 << 12) - 1);
        if (nn < b->ab_UnitMax && b->ab_Units[nn]) {
            struct virtio_Unit *unit = (struct virtio_Unit *)OOP_INST_DATA(VirtIOBase->unitClass, b->ab_Units[nn]);
            req->io_Unit  = &unit->au_Unit;
            req->io_Error = 0;
            AROS_ATOMIC_INC(unit->au_Unit.unit_OpenCnt);
            return TRUE;
        }
    }
    return FALSE;

    AROS_USERFUNC_EXIT
}
