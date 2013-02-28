/*
 * Copyright (C) 2012-2013, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#define __OOP_NOMETHODBASES__
 
#include "ahci_aros.h"

#define BUS_DMA_MAX_SEGMENTS    64

struct bus_dma_tag_slab {
    TAILQ_ENTRY(bus_dma_tag_slab) sl_node;
    APTR sl_memory;
    APTR sl_segment;
    int  sl_segfree;
    uint64_t sl_segmap;
};

struct bus_dma_tag {
    bus_size_t dt_boundary;
    bus_size_t dt_alignment;
    bus_size_t dt_maxsize;
    bus_size_t dt_maxsegsz;
    bus_size_t dt_segsize;
    int dt_nsegments;
    TAILQ_HEAD(, bus_dma_tag_slab) dt_slabs;
};

int bus_dma_tag_create(bus_dma_tag_t parent, bus_size_t alignment, bus_size_t boundary, bus_addr_t lowaddr, bus_addr_t highaddr, bus_dma_filter_t *filter, void *filterarg, bus_size_t maxsize, int nsegments, bus_size_t maxsegsz, int flags, bus_dma_tag_t *dmat)
{
    bus_dma_tag_t tag;

    D(bug("%s: Allocating tag, %d objects of size %d, aligned by %d\n", __func__, nsegments, maxsegsz, alignment));

    if (nsegments > BUS_DMA_MAX_SEGMENTS) {
        D(bug("%s: Too many segments, max is %d\n", __func__, BUS_DMA_MAX_SEGMENTS));
        return -EINVAL;
    }

    tag = AllocVec(sizeof(*tag), MEMF_ANY | MEMF_CLEAR);
    if (tag == NULL)
        return -ENOMEM;

    tag->dt_boundary = boundary;
    tag->dt_segsize = (maxsegsz + alignment-1) & ~(alignment - 1);
    tag->dt_nsegments = nsegments;
    tag->dt_alignment = alignment;
    tag->dt_maxsize = maxsize;
    tag->dt_maxsegsz = maxsegsz;

    TAILQ_INIT(&tag->dt_slabs);

    D(bug("%s: %p: Tag created\n", __func__, tag));

    (*dmat) = tag;
    return 0;
}

int bus_dma_tag_destroy(bus_dma_tag_t tag)
{
    struct bus_dma_tag_slab *slab;
    for (slab = TAILQ_FIRST(&tag->dt_slabs); slab;
         slab = TAILQ_FIRST(&tag->dt_slabs)) {
        TAILQ_REMOVE(&tag->dt_slabs, slab, sl_node);
        FreeVec(slab->sl_memory);
        FreeVec(slab);
    }
    FreeVec(tag);
    return 0;
}

static struct bus_dma_tag_slab *bus_dmamem_alloc_slab(bus_dma_tag_t tag)
{
    int boundary = tag->dt_boundary;
    struct bus_dma_tag_slab *slab;

    if (boundary < 4)
        boundary = 4;

    slab = AllocVec(sizeof(*slab), MEMF_ANY | MEMF_CLEAR);
    if (slab == NULL) {
        D(bug("%s: Can't allocate %d byte slab header\n", __func__, sizeof(*slab)));
        return NULL;
    }

    slab->sl_memory = AllocVec(tag->dt_segsize * tag->dt_nsegments + boundary - 4, MEMF_ANY);
    if (slab->sl_memory == NULL) {
        D(bug("%s: %p: Can't allocate %d bytes for DMA\n", __func__, tag, tag->dt_segsize * tag->dt_nsegments + boundary - 4));
        FreeVec(slab);
        return NULL;
    }

    slab->sl_segment = (APTR)(((IPTR)slab->sl_memory + boundary - 4) & ~(boundary-1));
    D(bug("%s: %p: Memory %p, %dx%d segments at %p\n", __func__, tag, slab->sl_memory, tag->dt_nsegments, tag->dt_maxsegsz, slab->sl_segment));

    slab->sl_segfree = tag->dt_nsegments;

    TAILQ_INSERT_TAIL(&tag->dt_slabs, slab, sl_node);

    return slab;
}

int bus_dmamem_alloc(bus_dma_tag_t tag, void **vaddr, unsigned flags, bus_dmamap_t *map)
{
    void *addr;
    struct bus_dma_tag_slab *slab;
    int i;

    TAILQ_FOREACH(slab, &tag->dt_slabs, sl_node) {
        if (slab->sl_segfree != 0)
            break;
    }

    if (slab == NULL) {
        slab = bus_dmamem_alloc_slab(tag);
        if (slab == NULL) {
            D(bug("%s: %p: Failed to allocate segment\n", __func__, tag));
            return -ENOMEM;
        }
    }

    D(bug("%s: %p: Slab %p 0x%08llx\n", __func__, tag, slab, slab->sl_segmap));
    for (i = 0; i < tag->dt_nsegments; i++ ) {
        if ((slab->sl_segmap & (1 << i)) == 0) {
            slab->sl_segmap |= (1 << i);
            slab->sl_segfree--;
            break;
        }
    }

    addr = slab->sl_segment + i * tag->dt_segsize;
    if (flags & MEMF_CLEAR)
        memset(addr, 0, tag->dt_segsize);

    D(bug("%s: %p: Allocated slot %d, %p: size %d\n", __func__, tag, i, addr, tag->dt_maxsegsz));
    if (vaddr)
        *vaddr = addr;

    *map = addr;
    return 0;
}

bus_size_t bus_dma_tag_getmaxsize(bus_dma_tag_t tag)
{
    return tag->dt_maxsize;
}

void bus_dmamem_free(bus_dma_tag_t tag, void *vaddr, bus_dmamap_t map)
{
    uintptr_t end_offset;
    struct bus_dma_tag_slab *slab;

    end_offset = tag->dt_segsize * (tag->dt_nsegments-1);

    TAILQ_FOREACH(slab, &tag->dt_slabs, sl_node) {
        if (vaddr >= slab->sl_segment && vaddr <= (slab->sl_segment + end_offset)) {
            int slot = (vaddr - slab->sl_segment) / tag->dt_segsize;
            assert(slab->sl_segmap & (1 << slot));
            slab->sl_segmap &= ~(1 << slot);
            slab->sl_segfree++;
            break;
        }
    }

    assert(slab != NULL);
}

int bus_dmamap_create(bus_dma_tag_t tag, unsigned flags, bus_dmamap_t *map)
{
    bus_dmamem_alloc(tag, NULL, 0, map);
    return 0;
}

void bus_dmamap_destroy(bus_dma_tag_t tag, bus_dmamap_t map)
{
    bus_dmamem_free(tag, NULL, map);
}

int bus_dmamap_load(bus_dma_tag_t tag, bus_dmamap_t map, void *data, size_t len, bus_dmamap_callback_t *callback, void *info, unsigned flags)
{
    bus_dma_segment_t seg = { .ds_addr = (bus_addr_t)data, .ds_len = (bus_size_t)len };
    callback(info, &seg, 1, 0);
    return 0;
}

void bus_dmamap_sync(bus_dma_tag_t tag, bus_dmamap_t map, unsigned flags)
{
    ULONG len = tag->dt_maxsegsz;

    if (!(flags & (1 << 31)))
        CachePreDMA(map, &len, flags);
    else
        CachePostDMA(map, &len, flags);
}

void bus_dmamap_unload(bus_dma_tag_t tag, bus_dmamap_t map)
{
}

struct resource *bus_alloc_resource_any(device_t dev, enum bus_resource_t type, int *rid, u_int flags)
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
    } else {
        /* FIXME: Map IRQ? */
        resource->res_handle = resource->res_tag;
        resource->res_size = 1;
    }

    return resource;
}

int bus_release_resource(device_t dev, enum bus_resource_t type, int rid, struct resource *res)
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

/* Bus IRQ */
AROS_INTH1(bus_intr_wrap, void **, fa)
{
    AROS_INTFUNC_INIT

    driver_intr_t *func = fa[0];
    void *arg =  fa[1];

    func(arg);

    return FALSE;

    AROS_INTFUNC_EXIT
}

int bus_setup_intr(device_t dev, struct resource *r, int flags, driver_intr_t func, void *arg, void **cookiep, void *serializer)
{
    struct AHCIBase *AHCIBase = dev->dev_AHCIBase;
    OOP_MethodID HiddPCIDeviceBase = AHCIBase->ahci_HiddPCIDeviceMethodBase;
    struct Interrupt *handler = AllocVec(sizeof(struct Interrupt)+sizeof(void *)*2, MEMF_PUBLIC | MEMF_CLEAR);
    void **fa;
    
    if (handler == NULL)
        return ENOMEM;

    handler->is_Node.ln_Pri = 10;
    handler->is_Node.ln_Name = device_get_name(dev);
    handler->is_Code = (VOID_FUNC)bus_intr_wrap;
    fa = (void **)&handler[1];
    fa[0] = func;
    fa[1] = arg;
    handler->is_Data = fa;

    if (!HIDD_PCIDevice_AddInterrupt(dev->dev_Object, handler))
    {
        FreeVec(handler);
        return ENOMEM;
    }

    *cookiep = handler;
    
    return 0;
}

int bus_teardown_intr(device_t dev, struct resource *r, void *cookie)
{
    struct AHCIBase *AHCIBase = dev->dev_AHCIBase;
    OOP_MethodID HiddPCIDeviceBase = AHCIBase->ahci_HiddPCIDeviceMethodBase;

    HIDD_PCIDevice_RemoveInterrupt(dev->dev_Object, cookie);
    FreeVec(cookie);

    return 0;
}
