/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>

#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>

#include "vmwgfx_drv.h"
#include "vmwgfx_bo.h"

/*
 * What the shared compatibility layer expects the hidd to supply, and the
 * few pieces of the driver that reach across to the hidd.
 */

/* --- allocator behind kmalloc ------------------------------------------- */

/*
 * Straight exec allocations: the drm code allocates under its "spinlocks"
 * (Forbid sections), where a pool semaphore could not wait. The 16-byte
 * header keeps the size and the alignment kmalloc callers expect. These
 * carry the names the shared linux_mem.c calls them by.
 */
#define VMWGFX_ALLOC_HEADER    16

APTR HIDDVMWareSVGAAlloc(ULONG size)
{
    IPTR total = (IPTR)size + VMWGFX_ALLOC_HEADER;
    IPTR *memory = AllocMem(total, MEMF_PUBLIC | MEMF_CLEAR);

    if (memory == NULL)
        return NULL;

    memory[0] = total;
    return (APTR)((UBYTE *)memory + VMWGFX_ALLOC_HEADER);
}

VOID HIDDVMWareSVGAFree(APTR memory)
{
    if (memory != NULL)
    {
        IPTR *real = (IPTR *)((UBYTE *)memory - VMWGFX_ALLOC_HEADER);
        FreeMem(real, real[0]);
    }
}

IPTR HIDDVMWareSVGAAllocSize(CONST_APTR memory)
{
    if (memory != NULL)
    {
        IPTR *real = (IPTR *)((UBYTE *)memory - VMWGFX_ALLOC_HEADER);
        return real[0] - VMWGFX_ALLOC_HEADER;
    }
    return 0;
}

/* a statistic the shared fence code reports; nothing feeds it here */
unsigned long vmwgfx_fence_uevent_irqs;

/* --- dma pools ------------------------------------------------------------ */

/*
 * The command buffer manager keeps its headers in a DMA pool. RAM is
 * identity mapped, so a pool is a plain aligned allocator whose bus
 * addresses come from the same translation every other mapping uses.
 */
struct dma_pool {
    struct device *dev;
    size_t size;
    size_t align;
};

struct dma_pool *dma_pool_create(const char *name, struct device *dev, size_t size, size_t align, size_t boundary)
{
    struct dma_pool *pool = kzalloc(sizeof(*pool), GFP_KERNEL);

    if (!pool)
        return NULL;
    pool->dev = dev;
    pool->size = size;
    pool->align = align ? align : 1;
    return pool;
}

void dma_pool_destroy(struct dma_pool *pool)
{
    kfree(pool);
}

void *dma_pool_alloc(struct dma_pool *pool, gfp_t mem_flags, dma_addr_t *handle)
{
    IPTR total = pool->size + pool->align + sizeof(IPTR) * 2;
    UBYTE *raw = AllocMem(total, MEMF_PUBLIC);
    UBYTE *aligned;
    IPTR *hdr;

    if (!raw)
        return NULL;

    aligned = (UBYTE *)(((IPTR)raw + sizeof(IPTR) * 2 + pool->align - 1) & ~(IPTR)(pool->align - 1));
    hdr = (IPTR *)aligned;
    hdr[-1] = (IPTR)raw;
    hdr[-2] = total;

    *handle = compat_dma_map(pool->dev, aligned, pool->size, DMA_BIDIRECTIONAL);
    return aligned;
}

void *dma_pool_zalloc(struct dma_pool *pool, gfp_t mem_flags, dma_addr_t *handle)
{
    void *p = dma_pool_alloc(pool, mem_flags, handle);

    if (p)
        memset(p, 0, pool->size);
    return p;
}

void dma_pool_free(struct dma_pool *pool, void *vaddr, dma_addr_t dma)
{
    IPTR *hdr = vaddr;

    if (!vaddr)
        return;
    FreeMem((APTR)hdr[-1], hdr[-2]);
}

/* --- buffer object mappings for the hidd ---------------------------------- */

/*
 * The hidd and the winsys hold CPU mappings of buffer objects directly;
 * there is no mmap offset to fault through. A map is the buffer's cached
 * kernel map, which the driver keeps for the life of the object (and
 * refreshes on a move).
 */
void *drm_gem_vmw_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle)
{
    struct drm_gem_object *gem_object;
    struct vmw_bo *vbo;
    void *addr = NULL;

    if (!f)
        return NULL;

    gem_object = drm_gem_object_lookup(f, handle);
    if (!gem_object)
        return NULL;

    vbo = to_vmw_bo(gem_object);
    if (vbo)
    {
        int ret = ttm_bo_reserve(&vbo->tbo, false, false, NULL);

        if (ret == 0)
        {
            addr = vmw_bo_map_and_cache(vbo);
            printk(KERN_DEBUG "vmwgfx: map bo %u: mem_type %u start %lu size %lu -> %p\n", handle,
                   vbo->tbo.resource->mem_type, (unsigned long)vbo->tbo.resource->start,
                   (unsigned long)vbo->tbo.resource->size, addr);
            ttm_bo_unreserve(&vbo->tbo);
        }
    }

    drm_gem_object_put(gem_object);
    return addr;
}

void drm_gem_vmw_munmap(struct drm_device *dev, struct drm_file *f, uint32_t handle)
{
    /* the cached map goes with the object */
}

/* --- what the hidd asks about the device --------------------------------- */

extern struct drm_device *current_drm_device;

BOOL vmwgfx_aros_has_3d(void)
{
    struct vmw_private *dev_priv;

    if (!current_drm_device)
        return FALSE;
    dev_priv = vmw_priv(current_drm_device);
    return vmw_supports_3d(dev_priv) ? TRUE : FALSE;
}

/* The KMS cursor plane needs guest backed objects to take a plain buffer */
BOOL vmwgfx_aros_has_mob(void)
{
    if (!current_drm_device)
        return FALSE;
    return vmw_priv(current_drm_device)->has_mob ? TRUE : FALSE;
}

/*
 * The system is resetting: put the device back the way the firmware
 * left it, without the unload's waits. The FIFO is drained first so
 * that nothing in flight lands after the registers change.
 */
void vmwgfx_aros_reset(void)
{
    struct vmw_private *dev_priv;

    if (!current_drm_device)
        return;
    dev_priv = vmw_priv(current_drm_device);

    vmw_write(dev_priv, SVGA_REG_SYNC, SVGA_SYNC_GENERIC);
    while (vmw_read(dev_priv, SVGA_REG_BUSY) != 0)
        ;
    vmw_write(dev_priv, SVGA_REG_CONFIG_DONE, dev_priv->config_done_state);
    vmw_write(dev_priv, SVGA_REG_ENABLE, dev_priv->enable_state);
    vmw_write(dev_priv, SVGA_REG_TRACES, dev_priv->traces_state);
}
