/*
 * Copyright (C) 2020-2025, The AROS Development Team.  All rights reserved.
 */

#ifndef NVME_INTERN_H
#define NVME_INTERN_H

#include <proto/exec.h>

#include <exec/types.h>
#include <asm/io.h>

#include <exec/memory.h>
#include <devices/scsidisk.h>
#include <exec/devices.h>

#include <oop/oop.h>
#include <utility/hooks.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>
#include <hardware/nvme.h>
#include <hidd/nvme.h>

#define USE_MSI

#if !defined(MIN)
# define MIN(a,b)       (a < b) ? a : b
#endif
#if !defined(MAX)
# define MAX(a,b)       (a > b) ? a : b
#endif

#define Unit(io) ((struct nvme_Unit *)(io)->io_Unit)
#define IOStdReq(io) ((struct IOStdReq *)io)

#define NVME_ID_CTRL_SGLS_OFFSET      536
#define NVME_ID_CTRL_SGLS_IO_COMMANDS (1UL << 0)

#define DMAFLAGS_PREREAD     0
#define DMAFLAGS_PREWRITE    DMA_ReadFromRAM
#define DMAFLAGS_POSTREAD    (1 << 31)
#define DMAFLAGS_POSTWRITE   (1 << 31) | DMA_ReadFromRAM

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
    APTR                        nvme_KernelBase;

    ULONG                       nvme_Flags;

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

#define NVMEB_FLAG_NOMSI        0
#define NVMEF_FLAG_NOMSI        (1 << NVMEB_FLAG_NOMSI)

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
#define KernelBase              (NVMEBase->nvme_KernelBase)

#include <exec/semaphores.h>

struct nvme_queue;

typedef struct {
    struct MinNode     	dev_Node;
    struct NVMEBase   	*dev_NVMEBase;
    OOP_Object        	*dev_Controller;		/* NVME HW Controller Object */
    OOP_Object        	*dev_Object;			/* PCI Device Object */
    OOP_Object        	*dev_PCIDriverObject;
    ULONG              	dev_HostID;

    UBYTE               dev_mdts;
    ULONG               dev_Features;

    int                 db_stride;
    volatile struct nvme_registers *dev_nvmeregbase;
    volatile ULONG *dbs;

    UWORD               pageshift;
    UWORD               pagesize;
    ULONG               ctrl_config;
    ULONG               queuecnt;
    struct nvme_queue   **dev_Queues;
} *device_t;

#define NVME_INLINE_DMA_SEGMENTS   4

#define NVME_DEVF_SGL_SUPPORTED   (1UL << 0)

struct nvme_dma_segment
{
    APTR                nds_Address;
    ULONG               nds_Length;
    ULONG               nds_Flags;
};

struct completionevent_handler
{
    struct Task         *ceh_Task;
    APTR                ceh_Msg;
    struct MemEntry     ceh_IOMem;
    ULONG               ceh_SigSet;
    ULONG               ceh_Result;
    volatile UWORD      ceh_Status;
    UWORD               ceh_Reply;

    struct nvme_dma_segment ceh_DMAInline[NVME_INLINE_DMA_SEGMENTS];
    struct nvme_dma_segment *ceh_DMAMap;
    ULONG               ceh_DMAMapCount;
    ULONG               ceh_DMAMapCapacity;
};

static inline void nvme_dma_init(struct completionevent_handler *handler)
{
    handler->ceh_DMAMap = handler->ceh_DMAInline;
    handler->ceh_DMAMapCount = 0;
    handler->ceh_DMAMapCapacity = NVME_INLINE_DMA_SEGMENTS;
}

static inline void nvme_dma_reset(struct completionevent_handler *handler)
{
    if (handler->ceh_DMAMap && handler->ceh_DMAMap != handler->ceh_DMAInline) {
        FreeMem(handler->ceh_DMAMap,
                handler->ceh_DMAMapCapacity * sizeof(struct nvme_dma_segment));
    }
    handler->ceh_DMAMap = handler->ceh_DMAInline;
    handler->ceh_DMAMapCapacity = NVME_INLINE_DMA_SEGMENTS;
    handler->ceh_DMAMapCount = 0;
}

static inline BOOL nvme_dma_ensure_capacity(struct completionevent_handler *handler,
                                            ULONG needed)
{
    if (needed <= handler->ceh_DMAMapCapacity) {
        return TRUE;
    }

    ULONG new_capacity = handler->ceh_DMAMapCapacity ? handler->ceh_DMAMapCapacity : NVME_INLINE_DMA_SEGMENTS;

    while (new_capacity < needed) {
        new_capacity <<= 1;
    }

    struct nvme_dma_segment *new_map = AllocMem(new_capacity * sizeof(struct nvme_dma_segment), MEMF_ANY | MEMF_CLEAR);
    if (!new_map) {
        return FALSE;
    }

    if (handler->ceh_DMAMapCount) {
        CopyMem(handler->ceh_DMAMap, new_map,
                handler->ceh_DMAMapCount * sizeof(struct nvme_dma_segment));
    }

    if (handler->ceh_DMAMap && handler->ceh_DMAMap != handler->ceh_DMAInline) {
        ULONG old_capacity = handler->ceh_DMAMapCapacity;
        FreeMem(handler->ceh_DMAMap,
                old_capacity * sizeof(struct nvme_dma_segment));
    }

    handler->ceh_DMAMap = new_map;
    handler->ceh_DMAMapCapacity = new_capacity;
    return TRUE;
}

static inline BOOL nvme_dma_append(struct completionevent_handler *handler,
                                   APTR address, ULONG length, ULONG flags)
{
    if (!nvme_dma_ensure_capacity(handler, handler->ceh_DMAMapCount + 1)) {
        return FALSE;
    }

    struct nvme_dma_segment *segment = &handler->ceh_DMAMap[handler->ceh_DMAMapCount++];
    segment->nds_Address = address;
    segment->nds_Length = length;
    segment->nds_Flags = flags;
    return TRUE;
}

static inline void nvme_dma_release(struct completionevent_handler *handler, BOOL do_post)
{
    if (do_post) {
        ULONG idx;

        for (idx = 0; idx < handler->ceh_DMAMapCount; idx++) {
            struct nvme_dma_segment *segment = &handler->ceh_DMAMap[idx];

            if (segment->nds_Address && segment->nds_Length) {
                ULONG length = segment->nds_Length;
                ULONG postflags = (segment->nds_Flags & DMAFLAGS_PREWRITE) ? DMAFLAGS_POSTWRITE : DMAFLAGS_POSTREAD;

                CachePostDMA(segment->nds_Address, &length, postflags);
            }
        }
    }

    if (handler->ceh_DMAMap && handler->ceh_DMAMap != handler->ceh_DMAInline) {
        ULONG allocated = handler->ceh_DMAMapCapacity;
        FreeMem(handler->ceh_DMAMap, allocated * sizeof(struct nvme_dma_segment));
    }

    handler->ceh_DMAMap = handler->ceh_DMAInline;
    handler->ceh_DMAMapCapacity = NVME_INLINE_DMA_SEGMENTS;
    handler->ceh_DMAMapCount = 0;
}

typedef void (*_NVMEQUEUE_CE_HOOK)(struct nvme_queue *, struct nvme_completion *);
struct nvme_queue {
    device_t dev;
    struct Task *q_IOTask;
    
#if defined(__AROSEXEC_SMP__)
    spinlock_t q_lock;
#endif
    struct Interrupt	q_IntHandler;
    volatile ULONG *q_db;
    UWORD q_depth;
    UWORD cq_vector;                                /* vector # - not the "vector" */
    UWORD q_irq;
    /* command queue */
    struct nvme_command *sqba;
    UQUAD sq_dma;
    UWORD sq_head;
    UWORD sq_tail;
    /* completion queue */
    _NVMEQUEUE_CE_HOOK *cehooks;
    struct completionevent_handler **cehandlers;
    struct completionevent_handler *ce_entries;
    volatile struct nvme_completion *cqba;
    UQUAD cq_dma;
    UWORD cq_head;
    UWORD cq_phase;
    UWORD cmdid_hint;
    UBYTE *cmdid_busy;
};

struct nvme_Controller
{
    struct Node         ac_Node;
    OOP_Class           *ac_Class;
    OOP_Object          *ac_Object;
    struct Interrupt    ac_ResetHandler;
    device_t            ac_dev;
};

struct nvme_Unit;

struct nvme_Bus
{
    struct NVMEBase     *ab_Base;   /* device self */
    device_t            ab_Dev;

    UWORD               ab_UnitMax;             /* Max units the bus can have   */
    UWORD               ab_UnitCnt;             /* actual # of units on the bus */
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

    ULONG               au_UnitNum;     /* Unit number as coded by device */

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

BOOL Hidd_NVMEBus_Start(OOP_Object *, struct NVMEBase *);
AROS_UFP3(BOOL, Hidd_NVMEBus_Open,
          AROS_UFPA(struct Hook *, h, A0),
          AROS_UFPA(OOP_Object *, obj, A2),
          AROS_UFPA(IPTR, reqUnit, A1));

#endif /* NVME_INTERN_H */
