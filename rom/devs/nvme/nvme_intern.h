/*
 * Copyright (C) 2020, The AROS Development Team.  All rights reserved.
 */

#ifndef NVME_INTERN_H
#define NVME_INTERN_H

#include <exec/types.h>
#include <asm/io.h>

#include <devices/scsidisk.h>
#include <exec/devices.h>

#include <oop/oop.h>
#include <utility/hooks.h>

#include <hardware/nvme.h>
#include <hidd/nvme.h>

#define Unit(io) ((struct nvme_Unit *)(io)->io_Unit)
#define IOStdReq(io) ((struct IOStdReq *)io)

/* nvme.device base */
struct NVMEBase
{
   /*
    * Device structure - used to manage devices by Exec
    */
   struct Device                nvme_Device;

   /*
    * master task pointer
    */
   struct Task                  *nvme_Daemon;

   /* Count of all hosts detected */
   ULONG                        nvme_HostCount;

   /*
    * List of all units
    */
   struct MinList               nvme_Units;

   /*
    * memory pool
    */
    APTR                        nvme_MemPool;

    struct Library              *nvme_OOPBase;
    struct Library              *nvme_UtilityBase;
#if defined(__AROSEXEC_SMP__)
    APTR                        nvme_KernelBase;
#endif
    /* Frequently used object offsets */
    OOP_Class                   *nvmeClass;
    OOP_Class                   *busClass;
    OOP_Class                   *unitClass;

    OOP_Object                  *storageRoot;

#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase                nvme_HWAttrBase;
    OOP_AttrBase                nvme_HiddAttrBase;
    OOP_AttrBase                nvme_HiddPCIDeviceAttrBase;
    OOP_AttrBase                nvme_HiddStorageUnitAttrBase;
    OOP_AttrBase                nvme_NVMEAttrBase;
    OOP_AttrBase                nvme_BusAttrBase;
    OOP_AttrBase                nvme_NVMEBusAttrBase;
    OOP_AttrBase                nvme_NVMEUnitAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID                nvme_HiddPCIDeviceMethodBase;
    OOP_MethodID                nvme_HiddPCIDriverMethodBase;
    OOP_MethodID                nvme_HWMethodBase;
    OOP_MethodID                nvme_SMethodBase;
    OOP_MethodID                nvme_SCMethodBase;
#endif
    struct List                 nvme_Controllers;
};

#if defined(__OOP_NOATTRBASES__)
#undef HWAttrBase
#define HWAttrBase     		(NVMEBase->nvme_HWAttrBase)
#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (NVMEBase->nvme_HiddPCIDeviceAttrBase)
#undef HiddBusAB
#define HiddBusAB   	        (NVMEBase->nvme_BusAttrBase)
#undef HiddNVMEBusAB
#define HiddNVMEBusAB   	(NVMEBase->nvme_NVMEBusAttrBase)
#undef HiddNVMEUnitAB
#define HiddNVMEUnitAB  	(NVMEBase->nvme_NVMEUnitAttrBase)
#undef HiddAttrBase
#define HiddAttrBase            (NVMEBase->nvme_HiddAttrBase)
#undef HiddStorageUnitAB
#define HiddStorageUnitAB       (NVMEBase->nvme_HiddStorageUnitAttrBase)
#endif
#if defined(__OOP_NOMETHODBASES__)
#undef HiddPCIDriverBase
#define HiddPCIDriverBase       (NVMEBase->nvme_HiddPCIDriverMethodBase)
#undef HiddPCIDeviceBase
#define HiddPCIDeviceBase       (NVMEBase->nvme_HiddPCIDeviceMethodBase)
#undef HWBase
#define HWBase                  (NVMEBase->nvme_HWMethodBase)
#undef HiddStorageBase
#define HiddStorageBase         (NVMEBase->nvme_SMethodBase)
#undef HiddStorageControllerBase
#define HiddStorageControllerBase       (NVMEBase->nvme_SCMethodBase)
#endif
#define OOPBase                 (NVMEBase->nvme_OOPBase)
#define UtilityBase             (NVMEBase->nvme_UtilityBase)
#if defined(__AROSEXEC_SMP__)
#define KernelBase              (NVMEBase->nvme_KernelBase)
#endif

#include <exec/semaphores.h>

struct nvme_queue;

typedef struct {
    struct MinNode     	dev_Node;
    struct NVMEBase   	*dev_NVMEBase;
    OOP_Object        	*dev_Controller;		/* NVME HW Controller Object */
    OOP_Object        	*dev_Object;			/* PCI Device Object */
    OOP_Object        	*dev_PCIDriverObject;
    ULONG              	dev_HostID;

    int                 db_stride;
    struct nvme_registers volatile *dev_nvmeregbase;
    ULONG volatile      *dbs;
    
    ULONG               ctrl_config;

    struct Interrupt	dev_AdminIntHandler;
    struct nvme_queue   *dev_AdminQueue;
} *device_t;

struct completionevent_handler
{
    struct Task *ceh_Task;
    ULONG ceh_SigSet;
    ULONG ceh_Result;
    UWORD ceh_Status;
};

typedef void (*_NVMEQUEUE_CE_HOOK)(struct nvme_queue *, struct nvme_completion *);
struct nvme_queue {
    device_t dev;
#if defined(__AROSEXEC_SMP__)
    spinlock_t q_lock;
#endif
    ULONG volatile *q_db;
    UWORD q_depth;
    UWORD cq_vector;
    /* command queue */
    struct nvme_command *sqba;
    UWORD sq_head;
    UWORD sq_tail;
    /* completion queue */
    _NVMEQUEUE_CE_HOOK *cehooks;
    struct completionevent_handler **cehandlers;
    volatile struct nvme_completion *cqba;
    UWORD cq_head;
    UWORD cq_phase;
    unsigned long cmdid_data;//[];
};

struct nvme_Controller
{
    struct Node         ac_Node;
    OOP_Class           *ac_Class;
    OOP_Object          *ac_Object;
    device_t            ac_dev;
    IPTR		ac_PCIIntLine;
};

struct nvme_Unit;

struct nvme_Bus
{
    struct NVMEBase     *ab_Base;   /* device self */

    device_t            ab_Dev;

    UWORD               ab_UnitCnt;
    OOP_Object          **ab_Units;

    struct Node         *ab_IDNode;

    char                *ab_DevName;
    char                *ab_DevSer;
    char                *ab_DevFW;
};

struct nvme_Unit
{
    struct Unit         au_Unit;        /* exec's unit */
    struct nvme_Bus     *au_Bus;         /* Bus on which this unit is */

    struct SignalSemaphore au_Lock;
    struct List         au_IOs;
    struct nvme_queue   *au_IOQueue;

    struct Interrupt	au_IOIntHandler;

    ULONG               au_UnitNum;     /* Unit number as coded by device */

    ULONG               au_SecShift;
    UQUAD               au_SecCnt;
    UQUAD               au_Low;
    UQUAD               au_High;

    UBYTE               au_Model[41];
    UBYTE               au_FirmwareRev[9];
    UBYTE               au_SerialNumber[21];
};

/* Function prototypes */

BOOL Hidd_NVMEBus_Start(OOP_Object *, struct NVMEBase *);
AROS_UFP3(BOOL, Hidd_NVMEBus_Open,
          AROS_UFPA(struct Hook *, h, A0),
          AROS_UFPA(OOP_Object *, obj, A2),
          AROS_UFPA(IPTR, reqUnit, A1));

#endif /* NVME_INTERN_H */
