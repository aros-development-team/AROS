/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 */

#ifndef VIRTIO_INTERN_H
#define VIRTIO_INTERN_H

#include <proto/exec.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <devices/scsidisk.h>

#include <oop/oop.h>
#include <utility/hooks.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>
#include <hardware/virtio.h>
#include <hidd/virtio.h>

#if !defined(MIN)
# define MIN(a,b)       (((a) < (b)) ? (a) : (b))
#endif
#if !defined(MAX)
# define MAX(a,b)       (((a) > (b)) ? (a) : (b))
#endif

#define Unit(io) ((struct virtio_Unit *)(io)->io_Unit)
#define IOStdReq(io) ((struct IOStdReq *)io)

/* virtio.device base */
struct VirtIOBase
{
    struct Device           virtio_Device;
    struct Task             *virtio_Daemon;
    ULONG                   virtio_HostCount;
    struct MinList          virtio_Units;
    APTR                    virtio_MemPool;

    struct Library          *virtio_OOPBase;
    struct Library          *virtio_UtilityBase;
    APTR                    virtio_KernelBase;

    ULONG                   virtio_Flags;

    OOP_Class               *virtioClass;
    OOP_Class               *busClass;
    OOP_Class               *unitClass;

    OOP_Object              *storageRoot;

#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase            virtio_HWAttrBase;
    OOP_AttrBase            virtio_HiddAttrBase;
    OOP_AttrBase            virtio_HiddPCIDeviceAttrBase;
    OOP_AttrBase            virtio_HiddStorageUnitAttrBase;
    OOP_AttrBase            virtio_VirtIOAttrBase;
    OOP_AttrBase            virtio_BusAttrBase;
    OOP_AttrBase            virtio_VirtIOBusAttrBase;
    OOP_AttrBase            virtio_VirtIOUnitAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID            virtio_HiddPCIDeviceMethodBase;
    OOP_MethodID            virtio_HiddPCIDriverMethodBase;
    OOP_MethodID            virtio_HWMethodBase;
    OOP_MethodID            virtio_SMethodBase;
    OOP_MethodID            virtio_SCMethodBase;
#endif
    struct List             virtio_Controllers;
};

#define VIRTIOB_FLAG_NOMSI      0
#define VIRTIOF_FLAG_NOMSI      (1 << VIRTIOB_FLAG_NOMSI)

#if defined(__OOP_NOATTRBASES__)
#undef HWAttrBase
#define HWAttrBase              (VirtIOBase->virtio_HWAttrBase)
#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (VirtIOBase->virtio_HiddPCIDeviceAttrBase)
#undef HiddBusAB
#define HiddBusAB               (VirtIOBase->virtio_BusAttrBase)
#undef HiddVirtIOBusAB
#define HiddVirtIOBusAB         (VirtIOBase->virtio_VirtIOBusAttrBase)
#undef HiddVirtIOUnitAB
#define HiddVirtIOUnitAB        (VirtIOBase->virtio_VirtIOUnitAttrBase)
#undef HiddAttrBase
#define HiddAttrBase            (VirtIOBase->virtio_HiddAttrBase)
#undef HiddStorageUnitAB
#define HiddStorageUnitAB       (VirtIOBase->virtio_HiddStorageUnitAttrBase)
#endif
#if defined(__OOP_NOMETHODBASES__)
#undef HiddPCIDriverBase
#define HiddPCIDriverBase       (VirtIOBase->virtio_HiddPCIDriverMethodBase)
#undef HiddPCIDeviceBase
#define HiddPCIDeviceBase       (VirtIOBase->virtio_HiddPCIDeviceMethodBase)
#undef HWBase
#define HWBase                  (VirtIOBase->virtio_HWMethodBase)
#undef HiddStorageBase
#define HiddStorageBase         (VirtIOBase->virtio_SMethodBase)
#undef HiddStorageControllerBase
#define HiddStorageControllerBase (VirtIOBase->virtio_SCMethodBase)
#endif
#define OOPBase                 (VirtIOBase->virtio_OOPBase)
#define UtilityBase             (VirtIOBase->virtio_UtilityBase)
#define KernelBase              (VirtIOBase->virtio_KernelBase)

struct virtio_queue;
struct virtio_Unit;

/*
 * Per-PCI-function device descriptor.
 */
typedef struct {
    struct MinNode      dev_Node;
    struct VirtIOBase   *dev_VirtIOBase;
    OOP_Object          *dev_Controller;
    OOP_Object          *dev_Object;
    OOP_Object          *dev_PCIDriverObject;
    ULONG               dev_HostID;

    UWORD               dev_VendorID;
    UWORD               dev_DeviceID;
    UWORD               dev_SubVendorID;
    UWORD               dev_SubsystemID;
    UBYTE               dev_IsModern;

    /* Mapped capability windows (modern transport) */
    volatile struct virtio_pci_common_cfg *dev_CommonCfg;
    volatile UBYTE                        *dev_DeviceCfg;
    volatile UBYTE                        *dev_NotifyBase;
    volatile UBYTE                        *dev_ISR;
    ULONG                                  dev_NotifyOffMultiplier;

    UQUAD               dev_Features;

    struct virtio_queue *dev_VQ;

    /* Cached device-cfg snapshot */
    UQUAD               dev_Capacity;       /* in 512-byte sectors */
    ULONG               dev_BlkSize;        /* logical block size in bytes */
    UWORD               dev_Cylinders;
    UBYTE               dev_Heads;
    UBYTE               dev_Sectors;
    UBYTE               dev_ReadOnly;
} *device_t;

/*
 * One descriptor chain in flight on the IO queue.
 */
struct virtio_request {
    struct IORequest        *vr_IORequest;
    struct virtio_blk_outhdr vr_Header;
    UBYTE                   vr_Status;
    UWORD                   vr_HeadDesc;
    UWORD                   vr_DescCount;
    UBYTE                   vr_InUse;
    UBYTE                   vr_IsWrite;
    APTR                    vr_DataAddr;
    ULONG                   vr_DataLength;
};

/*
 * Single split virtqueue.
 */
struct virtio_queue {
    device_t                dev;
    UWORD                   q_Index;
    UWORD                   q_Size;

    APTR                    q_RingBase;
    ULONG                   q_RingBytes;
    UQUAD                   q_DescPhys;
    UQUAD                   q_AvailPhys;
    UQUAD                   q_UsedPhys;

    volatile struct virtq_desc  *q_Desc;
    volatile struct virtq_avail *q_Avail;
    volatile struct virtq_used  *q_Used;

    UWORD                   q_FreeHead;
    UWORD                   q_NumFree;
    UWORD                   q_LastUsedIdx;

    struct virtio_request   *q_Reqs;

    volatile UWORD          *q_Notify;

    struct Interrupt        q_IntHandler;

    struct Task             *q_IOTask;

    struct SignalSemaphore  q_Lock;
};

struct virtio_Controller
{
    struct Node         ac_Node;
    OOP_Class           *ac_Class;
    OOP_Object          *ac_Object;
    struct Interrupt    ac_ResetHandler;
    device_t            ac_dev;
};

struct virtio_Bus
{
    struct VirtIOBase   *ab_Base;
    device_t            ab_Dev;

    UWORD               ab_UnitMax;
    UWORD               ab_UnitCnt;
    OOP_Object          **ab_Units;

    struct Node         *ab_IDNode;

    char                *ab_DevName;
    char                *ab_DevSer;
    char                *ab_DevFW;
};

struct virtio_Unit
{
    struct Unit         au_Unit;
    struct virtio_Bus   *au_Bus;

    struct SignalSemaphore au_Lock;
    struct List         au_IOs;

    ULONG               au_UnitNum;

    ULONG               au_SecShift;
    UQUAD               au_SecCnt;

    UQUAD               au_Low;
    UQUAD               au_High;

    ULONG               nu_Heads;
    ULONG               nu_Cyl;

    UBYTE               au_Model[41];
    UBYTE               au_FirmwareRev[9];
    UBYTE               au_SerialNumber[21];
};

/* Function prototypes */

BOOL Hidd_VirtIOBus_Start(OOP_Object *o, struct VirtIOBase *VirtIOBase);

AROS_UFP3(BOOL, Hidd_VirtIOBus_Open,
          AROS_UFPA(struct Hook *, h, A0),
          AROS_UFPA(OOP_Object *, obj, A2),
          AROS_UFPA(IPTR, reqUnit, A1));

#endif /* VIRTIO_INTERN_H */