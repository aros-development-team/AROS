/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef AHCI_AROS_H
#define AHCI_AROS_H

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/pci.h>

#include "ahci_intern.h"
#include "pci_ids.h"

#undef kprintf
#define kprintf(fmt, args...) device_printf(NULL, fmt ,##args)

#ifdef __GNUC__
#define __packed __attribute__((__packed__))
#else
#error Define __packed appropriately for your compiler!
#endif

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
typedef unsigned int u_int;

#define le16toh(x)      AROS_LE2WORD(x)
#define le32toh(x)      AROS_LE2LONG(x)
#define htole32(x)      AROS_LONG2LE(x)
#define htole16(x)      AROS_WORD2LE(x)

#define PAGE_SIZE       4096

typedef struct {
    struct MinNode dev_Node;
    OOP_Object *dev_Object;
    struct AHCIBase *dev_AHCIBase;
    struct ahci_softc *dev_softc;
} *device_t;

/* Kernel stuff */

#define KKASSERT(expr)  do { if (expr); } while (0)

int kvsnrprintf(char *str, size_t size, int radix, const char *format, va_list ap);
int kvsnprintf(char *str, size_t size, const char *format, va_list ap);
int ksnprintf(char *buff, size_t len, const char *fmt, ...);
int kvcprintf(char const *fmt, void (*func)(int, void*), void *arg, int radix, va_list ap);

static inline void bug_c(int c, void *info)
{
    RawPutChar(c);
}

static inline int device_printf(device_t dev, const char *fmt, ...)
{
    va_list args;
    int err;
    va_start(args, fmt);
    err = kvcprintf(fmt, bug_c, NULL, 10, args);
    va_end(args);
    return err;
}


#define panic(fmt, args...) do { Forbid(); device_printf(NULL, fmt ,##args); Disable(); for (;;); } while (0);

static inline void *kmalloc(size_t size, unsigned where, unsigned flags)
{
    return AllocVec(size, flags);
}

static inline void kfree(void *ptr, unsigned where)
{
    FreeVec(ptr);
}

static inline void crit_enter(void)
{
    Disable();
}

static inline void crit_exit(void)
{
    Enable();
}

typedef struct Task *thread_t;

static inline int kthread_create(void (*func)(void *), void *arg, thread_t *tdp, const char *fmt, ...)
{
    va_list args;
    char name[64];

    va_start(args, fmt);
    kvsnprintf(name, sizeof(name), fmt, args);
    va_end(args);

    name[sizeof(name)-1] = 0;

    *tdp = NewCreateTask(TASKTAG_NAME, name,
                         TASKTAG_PC, func,
                         TASKTAG_PRI, 20,
                         TASKTAG_ARG1, arg,
                         TAG_END);

    return (*tdp == NULL) ? ENOMEM : 0;
}

/* PCI devices */
typedef u_int16_t pci_vendor_id_t;
typedef u_int16_t pci_product_id_t;
typedef u_int32_t pcireg_t;

static inline u_int32_t pci_read_config(device_t dev, int reg, int width)
{
    u_int32_t val = ~0;
    struct AHCIBase *AHCIBase = dev->dev_AHCIBase;
    OOP_MethodID HiddPCIDeviceMethodBase = AHCIBase->ahci_HiddPCIDeviceMethodBase;
    struct pHidd_PCIDevice_ReadConfigByte cb; 
    struct pHidd_PCIDevice_ReadConfigWord cw; 
    struct pHidd_PCIDevice_ReadConfigLong cl; 
    OOP_Object *Device = dev->dev_Object;

    switch (width) {
    case 1:
        cb.mID = HiddPCIDeviceMethodBase + moHidd_PCIDevice_ReadConfigByte;
        cb.reg = reg;
        val = (u_int32_t)OOP_DoMethod(Device, (OOP_Msg)&cb);
        break;

    case 2:
        cw.mID = HiddPCIDeviceMethodBase + moHidd_PCIDevice_ReadConfigWord;
        cw.reg = reg;
        val = (u_int32_t)OOP_DoMethod(Device, (OOP_Msg)&cw);
        break;

    case 4:
        cl.mID = HiddPCIDeviceMethodBase + moHidd_PCIDevice_ReadConfigLong;
        cl.reg = reg;
        val = (u_int32_t)OOP_DoMethod(Device, (OOP_Msg)&cl);
        break;
    }

    return val;
}

static inline void pci_write_config(device_t dev, int reg, u_int32_t val, int width)
{
    struct AHCIBase *AHCIBase = dev->dev_AHCIBase;
    OOP_MethodID HiddPCIDeviceMethodBase = AHCIBase->ahci_HiddPCIDeviceMethodBase;
    struct pHidd_PCIDevice_WriteConfigByte cb; 
    struct pHidd_PCIDevice_WriteConfigWord cw; 
    struct pHidd_PCIDevice_WriteConfigLong cl; 
    OOP_Object *Device = dev->dev_Object;

    switch (width) {
    case 1:
        cb.mID = HiddPCIDeviceMethodBase + moHidd_PCIDevice_WriteConfigByte;
        cb.reg = reg;
        cw.val = val & 0xff;
        OOP_DoMethod(Device, (OOP_Msg)&cb);
        break;

    case 2:
        cw.mID = HiddPCIDeviceMethodBase + moHidd_PCIDevice_WriteConfigWord;
        cw.reg = reg;
        cw.val = val & 0xffff;
        OOP_DoMethod(Device, (OOP_Msg)&cw);
        break;

    case 4:
        cl.mID = HiddPCIDeviceMethodBase + moHidd_PCIDevice_WriteConfigLong;
        cl.reg = reg;
        cl.val = val;
        OOP_DoMethod(Device, (OOP_Msg)&cl);
        break;
    }

}

static inline u_int16_t pci_get_vendor(device_t dev)
{
    return (u_int16_t)pci_read_config(dev, PCIR_VENDOR, 2);
}

static inline u_int16_t pci_get_device(device_t dev)
{
    return (u_int16_t)pci_read_config(dev, PCIR_DEVICE, 2);
}

static inline u_int8_t pci_get_class(device_t dev)
{
    return (u_int8_t)pci_read_config(dev, PCIR_CLASS, 1);
}

static inline u_int8_t pci_get_subclass(device_t dev)
{
    return (u_int8_t)pci_read_config(dev, PCIR_SUBCLASS, 1);
}

/* DMA Types */
typedef IPTR bus_size_t;
typedef IPTR bus_addr_t;
typedef struct {
    bus_addr_t ds_addr;
    bus_size_t ds_len;
} bus_dma_segment_t;

typedef struct {
    bus_size_t dt_size;
} *bus_dma_tag_t;

typedef IPTR bus_space_tag_t;
typedef APTR bus_dmamap_t;
typedef IPTR bus_space_handle_t;
typedef int bus_dma_filter_t(void *arg, bus_addr_t paddr);

#define BUS_SPACE_MAXADDR       ~0
#define BUS_SPACE_MAXADDR_32BIT ((ULONG)~0)

static inline int bus_dma_tag_create(bus_dma_tag_t parent, bus_size_t alignment, bus_size_t boundary, bus_addr_t lowaddr, bus_addr_t highaddr, bus_dma_filter_t *filter, void *filterarg, bus_size_t maxsize, int nsegments, bus_size_t maxsegsz, int flags, bus_dma_tag_t *dmat)
{
    *dmat = AllocVec(sizeof(**dmat), MEMF_ANY);
    if (*dmat == NULL)
        return -ENOMEM;

    (*dmat)->dt_size = maxsize;
    return 0;
}

static inline int bus_dma_tag_destroy(bus_dma_tag_t tag)
{
    FreeVec(tag);
    return 0;
}

#define BUS_DMA_ALLOCNOW (1 << 0)
#define BUS_DMA_ZERO     (1 << 1)

static inline int bus_dmamem_alloc(bus_dma_tag_t tag, void **vaddr, unsigned flags, bus_dmamap_t *map)
{
    /* FIXME: Allocate DMAable memory */
    void *addr = AllocMem(tag->dt_size, MEMF_ANY);
    if (addr == NULL)
        return -ENOMEM;

    if (vaddr)
        *vaddr = addr;

    *map = addr;
    return 0;
}

static inline bus_size_t bus_dma_tag_getmaxsize(bus_dma_tag_t tag)
{
    return tag->dt_size;
}

static inline void bus_dmamem_free(bus_dma_tag_t tag, void *vaddr, bus_dmamap_t map)
{
    FreeMem(map, tag->dt_size);
}

static inline int bus_dmamap_create(bus_dma_tag_t tag, unsigned flags, bus_dmamap_t *map)
{
    bus_dmamem_alloc(tag, NULL, 0, map);
    return 0;
}

static inline void bus_dmamap_destroy(bus_dma_tag_t tag, bus_dmamap_t map)
{
    bus_dmamem_free(tag, NULL, map);
}

typedef void bus_dmamap_callback_t(void *info, bus_dma_segment_t *segs, int nsegs, int error);

#define BUS_DMA_NOWAIT  0
#define BUS_DMA_WAITOK  0
#define BUS_DMASYNC_POSTREAD 0
#define BUS_DMASYNC_POSTWRITE 0

static inline int bus_dmamap_load(bus_dma_tag_t tag, bus_dmamap_t map, void *data, size_t len, bus_dmamap_callback_t *callback, void *info, unsigned flags)
{
    bus_dma_segment_t seg = { .ds_addr = (bus_addr_t)data, .ds_len = (bus_size_t)len };
    callback(info, &seg, 1, 0);
    return 0;
}

#define BUS_DMASYNC_PREREAD     0
#define BUS_DMASYNC_PREWRITE    0

static inline void bus_dmamap_sync(bus_dma_tag_t tag, bus_dmamap_t map, unsigned flags)
{
}

static inline void bus_dmamap_unload(bus_dma_tag_t tag, bus_dmamap_t map)
{
}

/* Generic bus operations */
enum bus_resource_t {
    SYS_RES_IRQ = 0,
    SYS_RES_MEMORY = 1,
};

#define AHCI_IRQ_RID    0

struct resource {
    bus_space_tag_t res_tag;
    bus_space_handle_t res_handle;
    ULONG res_size;
};

#define RF_SHAREABLE    (1 << 0)
#define RF_ACTIVE       (1 << 1)

static inline struct resource *bus_alloc_resource_any(device_t dev, enum bus_resource_t type, int *rid, u_int flags)
{
    struct resource *resource;
    IPTR INTLine;
    OOP_AttrBase HiddPCIDeviceAttrBase = dev->dev_AHCIBase->ahci_HiddPCIDeviceAttrBase;

    resource = AllocPooled(dev->dev_AHCIBase->ahci_MemPool, sizeof(*resource));
    if (!resource)
        return NULL;

    switch (type) {
    case SYS_RES_IRQ:
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_INTLine, &INTLine);
        resource->res_tag = INTLine;
        break;
    case SYS_RES_MEMORY:
        resource->res_tag = pci_read_config(dev, *rid, 4);
        break;
    }

    if (type == SYS_RES_MEMORY && (*rid) >= PCIR_BAR(0) && (*rid) < PCIR_BAR(6)) {
        struct pHidd_PCIDriver_MapPCI map;
        IPTR hba_size;
        OOP_Object *Driver;

        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_Driver, (IPTR *)&Driver);
        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_Size0 + (((*rid) - PCIR_BAR(0))/4)*3, &hba_size);
        resource->res_size = hba_size;

        map.mID = dev->dev_AHCIBase->ahci_HiddPCIDriverMethodBase + moHidd_PCIDriver_MapPCI;
        map.PCIAddress = (APTR)resource->res_tag;
        map.Length = resource->res_size;
        resource->res_handle = OOP_DoMethod(Driver, (OOP_Msg)&map);
        device_printf(dev, "Map 0x%x %p => %p (%d)\n", *rid, resource->res_tag, resource->res_handle, resource->res_size);
    } else {
        /* FIXME: Map IRQ? */
        resource->res_handle = resource->res_tag;
        resource->res_size = 1;
        device_printf(dev, "IRQ %d %p\n", *rid, resource->res_tag);
    }

    return resource;
}

static inline int bus_release_resource(device_t dev, enum bus_resource_t type, int rid, struct resource *res)
{
    OOP_AttrBase HiddPCIDeviceAttrBase = dev->dev_AHCIBase->ahci_HiddPCIDeviceAttrBase;

    if (type == SYS_RES_MEMORY && rid > PCIR_BAR(0) && rid < PCIR_BAR(6)) {
        struct pHidd_PCIDriver_UnmapPCI unmap;
        OOP_Object *Driver;

        OOP_GetAttr(dev->dev_Object, aHidd_PCIDevice_Driver, (IPTR *)&Driver);
        unmap.mID = dev->dev_AHCIBase->ahci_HiddPCIDriverMethodBase + moHidd_PCIDriver_UnmapPCI;
        unmap.CPUAddress = (APTR)res->res_handle;
        unmap.Length = res->res_size;
        OOP_DoMethod(Driver, (OOP_Msg)&unmap);
    }
    FreePooled(dev->dev_AHCIBase->ahci_MemPool, res, sizeof(*res));
    return 0;
}


static inline bus_space_tag_t rman_get_bustag(struct resource *r)
{
    return r->res_tag;
}

static inline bus_space_handle_t rman_get_bushandle(struct resource *r)
{
    return r->res_handle;
}

/* Bus IRQ */
typedef void driver_intr_t(void *arg);

#define INTR_MPSAFE 0

int bus_setup_intr(device_t dev, struct resource *r, int flags, driver_intr_t handler, void *arg, void **cookiep, void *serializer);

int bus_teardown_intr(device_t dev, struct resource *r, void *cookie);

/* Bus IO */

static inline int bus_space_subregion(bus_space_tag_t iot, bus_space_handle_t ioh, unsigned offset, size_t size, bus_space_handle_t *result)
{
    *result = ioh + offset;
    return 0;
}

#define BUS_SPACE_BARRIER_READ  0
#define BUS_SPACE_BARRIER_WRITE 0

static inline void bus_space_barrier(bus_space_tag_t iot, bus_space_handle_t ioh, unsigned offset, size_t size, unsigned flags)
{
    /* FIXME: Sync bus area */
}

static inline u_int32_t bus_space_read_4(bus_space_tag_t iot, bus_space_handle_t ioh, unsigned offset)
{
    return *(u_int32_t *)(ioh + offset);
}

static inline void bus_space_write_4(bus_space_tag_t iot, bus_space_handle_t ioh, unsigned offset, u_int32_t val)
{
    *(u_int32_t *)(ioh + offset) = val;
}


/* Generic device info */
static inline void *device_get_softc(device_t dev)
{
    return dev->dev_softc;
}

/* Lock management */

struct lock {
    struct SignalSemaphore sem;
    const char *name;
};

static inline void lockinit(struct lock *lock, const char *name, unsigned flags, unsigned count)
{
    lock->name = name;
    InitSemaphore(&lock->sem);
}

static inline void lockuninit(struct lock *lock)
{
    /* Nothing needed */
}

#define LK_RELEASE    (1 << 0)
#define LK_EXCLUSIVE  (1 << 1)
#define LK_CANRECURSE (1 << 2)
#define LK_NOWAIT     (1 << 3)

static inline int lockmgr(struct lock *lock, int flags)
{
    int err = 0;

    flags &= (LK_EXCLUSIVE | LK_NOWAIT | LK_RELEASE);
    switch (flags) {
    case 0:
        ObtainSemaphoreShared(&lock->sem);
        break;
    case LK_EXCLUSIVE | LK_NOWAIT:
        err = (AttemptSemaphore(&lock->sem) == FALSE) ? 1 : 0;
        break;
    case LK_EXCLUSIVE:
        ObtainSemaphore(&lock->sem);
        break;
    case LK_RELEASE:
        ReleaseSemaphore(&lock->sem);
        break;
    }

    return err;
}

/* Events */
#define atomic_clear_int(ptr, val) AROS_ATOMIC_AND(*ptr, ~val)
#define atomic_set_int(ptr, val) AROS_ATOMIC_OR(*ptr, val)

/* Callouts */

static const int hz = (1000000 / 2); /* UNIT_MICROHZ frequency */

typedef void timeout_t (void *);

struct callout {
    struct Task *co_Task;
};

void callout_init(struct callout *c);

void callout_stop(struct callout *c);

int callout_reset(struct callout *c, unsigned int ticks, void (*func)(void *), void *arg);

struct sysctl_ctx_list {};

/* BSD style TailQs */
#define TAILQ_HEAD(sname,type)  struct sname { struct type *next, *last; }
#define TAILQ_ENTRY(type)       TAILQ_HEAD(,type)
#define TAILQ_INIT(head)        do { (head)->next = (head)->last = NULL; } while (0)
#define TAILQ_FIRST(head)       ((head)->next)
#define TAILQ_REMOVE(head,n,link)  ({ (n) = (head)->next; if ((n) != NULL) (head)->next = (n)->link.next; (n)->link.next = (n)->link.last = NULL; })
#define TAILQ_INSERT_TAIL(head,n,link)  do { if ((head)->last != NULL) (head)->last->link.next = (n); (head)->last = (n); if ((head)->next == NULL) (head)->next = (n); } while (0)

#define device_get_name(dev)    "ahci"
#define device_get_unit(dev)    0

#define M_DEVBUF        0
#define M_TEMP          0
#define M_WAITOK        0
#define M_INTWAIT       0
#define M_ZERO          MEMF_CLEAR

static const int bootverbose = 0;

/* Bit operations */
static inline int ffs(unsigned int bits)
{
    int i;

    for (i = 0; i < 32; i++, bits >>= 1)
        if (bits & 1)
            return (i+1);

    return 0;
}

struct ata_xfer;
void ahci_ata_io_complete(struct ata_xfer *xa);

#endif /* AHCI_AROS_H */
