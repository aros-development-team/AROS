/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    V3D DRM ioctl shim: what Mesa's v3d driver reaches on AROS in place of
    a host kernel's DRM device.

    Follows vc4gallium's hardware-proven model. Buffer objects are
    firmware allocations (ALLOCMEM/LOCKMEM) reached through the uncached
    bus alias, so the CPU and the GPU agree on memory contents without a
    single cache operation; handles come from a rotating table that reuses
    freed slots. The dispatch switches on the DRM_IOCTL_* constants from
    the same drm-uapi header Mesa compiles against - the first version of
    this file kept a private copy of both the numbers and the structs, and
    the numbers had drifted a COMMAND_BASE apart from what Mesa sent.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/mbox.h>

#include "v3d_intern.h"

#include <stdint.h>
#include "drm-uapi/v3d_drm.h"

#define MBoxBase (sd->mbox_base)

#define VCMB_BASE       (ARM_PERIIOBASE + 0xB880)
#define VCMB_PROPCHAN   8
#define VCTAG_REQ       0
#define VCTAG_ALLOCMEM  0x0003000c
#define VCTAG_LOCKMEM   0x0003000d
#define VCTAG_UNLOCKMEM 0x0003000e
#define VCTAG_FREEMEM   0x0003000f

#define VCMEM_DIRECT    (1 << 2)
#define VCMEM_ZERO      (1 << 4)

/* Global for the drmIoctl override macro; set at CreatePipeScreen. */
struct V3DData *g_v3d_data = NULL;

/* ---- GPU memory, through the firmware allocator ---- */

ULONG v3d_gpu_mem_alloc(struct V3DData *sd, ULONG size, ULONG align,
                        ULONG *out_paddr)
{
    volatile ULONG *msg = sd->mbox_msg;
    ULONG gpu_handle, paddr;

    ObtainSemaphore(&sd->mbox_lock);

    msg[0] = AROS_LE2LONG(9 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_ALLOCMEM);
    msg[3] = AROS_LE2LONG(12);
    msg[4] = AROS_LE2LONG(12);
    msg[5] = AROS_LE2LONG(size);
    msg[6] = AROS_LE2LONG(align);
    msg[7] = AROS_LE2LONG(VCMEM_DIRECT | VCMEM_ZERO);
    msg[8] = 0;

    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg)
        == (volatile unsigned int *)-1
        || (gpu_handle = AROS_LE2LONG(msg[5])) == 0)
    {
        ReleaseSemaphore(&sd->mbox_lock);
        bug("[V3D] ALLOCMEM failed for %u bytes (gpu_mem= too small?)\n",
            (unsigned)size);
        return 0;
    }

    msg[0] = AROS_LE2LONG(8 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_LOCKMEM);
    msg[3] = AROS_LE2LONG(4);
    msg[4] = AROS_LE2LONG(4);
    msg[5] = AROS_LE2LONG(gpu_handle);
    msg[6] = 0;
    msg[7] = 0;

    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg)
        == (volatile unsigned int *)-1
        || (paddr = AROS_LE2LONG(msg[5]) & 0x3fffffff) == 0)
    {
        ReleaseSemaphore(&sd->mbox_lock);
        v3d_gpu_mem_free(sd, gpu_handle);
        bug("[V3D] LOCKMEM failed for handle %u\n", (unsigned)gpu_handle);
        return 0;
    }

    ReleaseSemaphore(&sd->mbox_lock);
    *out_paddr = paddr;
    return gpu_handle;
}

void v3d_gpu_mem_free(struct V3DData *sd, ULONG gpu_handle)
{
    volatile ULONG *msg = sd->mbox_msg;

    ObtainSemaphore(&sd->mbox_lock);

    msg[0] = AROS_LE2LONG(8 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_UNLOCKMEM);
    msg[3] = AROS_LE2LONG(4);
    msg[4] = AROS_LE2LONG(4);
    msg[5] = AROS_LE2LONG(gpu_handle);
    msg[6] = 0;
    msg[7] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);

    msg[0] = AROS_LE2LONG(8 * 4);
    msg[1] = AROS_LE2LONG(VCTAG_REQ);
    msg[2] = AROS_LE2LONG(VCTAG_FREEMEM);
    msg[3] = AROS_LE2LONG(4);
    msg[4] = AROS_LE2LONG(4);
    msg[5] = AROS_LE2LONG(gpu_handle);
    msg[6] = 0;
    msg[7] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)msg);

    ReleaseSemaphore(&sd->mbox_lock);
}

/* ---- BO handle table (rotating, slots reused on free) ---- */

static ULONG bo_alloc_handle(struct V3DData *sd)
{
    ULONG i;

    for (i = 1; i < V3D_MAX_BOS; i++)
    {
        ULONG h = (sd->bo_next_handle + i) % V3D_MAX_BOS;

        if (h == 0)
            continue;
        if (sd->bo_table[h].refcount == 0)
        {
            sd->bo_next_handle = h;
            return h;
        }
    }
    return 0;
}

static struct V3DBO *bo_lookup(struct V3DData *sd, ULONG handle)
{
    if (handle == 0 || handle >= V3D_MAX_BOS
        || sd->bo_table[handle].refcount == 0)
        return NULL;
    return &sd->bo_table[handle];
}

static void bo_unref(struct V3DData *sd, ULONG handle)
{
    struct V3DBO *bo = bo_lookup(sd, handle);

    if (!bo)
        return;
    if (--bo->refcount == 0)
    {
        v3d_mmu_unmap(sd, bo->gpu_va, bo->size);
        if (!bo->external)
            v3d_gpu_mem_free(sd, bo->gpu_handle);
        bo->gpu_handle = 0;
        bo->external = FALSE;
    }
}

/*
 * Session sweep, called when the last pipe screen goes: whatever Mesa
 * leaked stays firmware-LOCKED forever otherwise, and the GPU heap
 * shrinks with every GL session until allocations start failing. The
 * driver's own allocations (page table, scratch, overflow, landing
 * zone) never sit in this table, so everything here belongs to Mesa.
 */
void v3d_release_all_bos(struct V3DData *sd)
{
    ULONG h, leaked = 0;

    ObtainSemaphore(&sd->bo_lock);
    for (h = 1; h < V3D_MAX_BOS; h++)
    {
        struct V3DBO *bo = &sd->bo_table[h];

        if (bo->refcount == 0)
            continue;
        v3d_mmu_unmap(sd, bo->gpu_va, bo->size);
        if (!bo->external)
            v3d_gpu_mem_free(sd, bo->gpu_handle);
        bo->gpu_handle = 0;
        bo->refcount = 0;
        bo->external = FALSE;
        leaked++;
    }
    sd->bo_next_handle = 0;
    ReleaseSemaphore(&sd->bo_lock);

    if (leaked)
        bug("[V3D] session sweep: released %u leaked BOs\n", (unsigned)leaked);
}

/* ---- the dispatch ---- */

int v3d_ioctl_aros(struct V3DData *sd, unsigned long request, void *arg)
{
    if (!sd)
        return -1;

    switch (request)
    {
    case DRM_IOCTL_V3D_CREATE_BO:
    {
        struct drm_v3d_create_bo *create = arg;
        ULONG paddr, gpu_handle, h;

        ObtainSemaphore(&sd->bo_lock);
        h = bo_alloc_handle(sd);
        if (!h)
        {
            ReleaseSemaphore(&sd->bo_lock);
            bug("[V3D] out of BO handles\n");
            return -1;
        }

        /* 4K-aligned: the page granularity the driver's BO handling and
         * the MMU's page table both assume. */
        gpu_handle = v3d_gpu_mem_alloc(sd, create->size, 4096, &paddr);
        if (!gpu_handle)
        {
            ReleaseSemaphore(&sd->bo_lock);
            return -1;
        }

        sd->bo_table[h].gpu_handle = gpu_handle;
        sd->bo_table[h].paddr      = paddr;
        sd->bo_table[h].gpu_va     = v3d_mmu_map(sd, paddr, create->size);
        sd->bo_table[h].size       = create->size;
        sd->bo_table[h].refcount   = 1;
        sd->bo_table[h].external   = FALSE;
        ReleaseSemaphore(&sd->bo_lock);

        create->handle = h;
        create->offset = sd->bo_table[h].gpu_va;
        return 0;
    }

    case DRM_IOCTL_GEM_OPEN:
    {
        /* Wrap a framebuffer flip page as a BO, so Mesa renders straight
         * into scanout and presenting becomes a page flip. The only
         * names accepted are the pages the gallium class published; the
         * memory is not ours, so the entry is marked external and only
         * ever unmapped, never FREEMEMed. */
        struct drm_gem_open *open = arg;
        ULONG name = (ULONG)open->name, h;

        ObtainSemaphore(&sd->bo_lock);
        if (!sd->scanout_size
            || (name != sd->scanout_phys[0] && name != sd->scanout_phys[1]))
        {
            ReleaseSemaphore(&sd->bo_lock);
            return -1;
        }

        /* The same page opened again keeps its handle - Mesa's handle
         * hash relies on that identity. */
        for (h = 1; h < V3D_MAX_BOS; h++)
            if (sd->bo_table[h].refcount && sd->bo_table[h].external
                && sd->bo_table[h].paddr == name)
                break;

        if (h < V3D_MAX_BOS)
            sd->bo_table[h].refcount++;
        else
        {
            h = bo_alloc_handle(sd);
            if (!h)
            {
                ReleaseSemaphore(&sd->bo_lock);
                bug("[V3D] out of BO handles\n");
                return -1;
            }
            sd->bo_table[h].gpu_handle = 0;
            sd->bo_table[h].external   = TRUE;
            sd->bo_table[h].paddr      = name;
            sd->bo_table[h].gpu_va     = v3d_mmu_map(sd, name,
                                                     sd->scanout_size);
            sd->bo_table[h].size       = sd->scanout_size;
            sd->bo_table[h].refcount   = 1;
        }
        open->handle = h;
        open->size = sd->bo_table[h].size;
        ReleaseSemaphore(&sd->bo_lock);
        return 0;
    }

    case DRM_IOCTL_GEM_CLOSE:
    {
        struct drm_gem_close *close = arg;

        /* With jobs in flight the GPU may still read this BO - no
         * per-job references exist, so drain first. Rare: Mesa's BO
         * cache absorbs the per-frame churn, real closes are eviction. */
        if (sd->finished_seqno != sd->seqno)
            v3d_wait_idle(sd);

        ObtainSemaphore(&sd->bo_lock);
        bo_unref(sd, close->handle);
        ReleaseSemaphore(&sd->bo_lock);
        return 0;
    }

    case DRM_IOCTL_V3D_MMAP_BO:
    {
        struct drm_v3d_mmap_bo *map = arg;
        struct V3DBO *bo;

        ObtainSemaphore(&sd->bo_lock);
        bo = bo_lookup(sd, map->handle);
        if (bo)
            map->offset = bo->paddr;    /* mmap() hands this back */
        ReleaseSemaphore(&sd->bo_lock);
        return bo ? 0 : -1;
    }

    case DRM_IOCTL_V3D_GET_BO_OFFSET:
    {
        struct drm_v3d_get_bo_offset *get = arg;
        struct V3DBO *bo;

        ObtainSemaphore(&sd->bo_lock);
        bo = bo_lookup(sd, get->handle);
        if (bo)
            get->offset = bo->gpu_va;
        ReleaseSemaphore(&sd->bo_lock);
        return bo ? 0 : -1;
    }

    case DRM_IOCTL_V3D_WAIT_BO:
        /* No per-BO tracking: any BO may belong to the jobs in flight,
         * so waiting on one means draining the pipeline. Mesa only asks
         * before CPU access, which is rare on the hot path. */
        v3d_wait_idle(sd);
        return 0;

    case DRM_IOCTL_V3D_SUBMIT_CL:
    {
        struct drm_v3d_submit_cl *submit = arg;

        /* Asynchronous: returns once the bin job is kicked; the render
         * is stashed and handed over on the binner's flush. */
        v3d_submit_cl(sd, submit->bcl_start, submit->bcl_end,
                      submit->qma, submit->qms, submit->qts,
                      submit->rcl_start, submit->rcl_end);
        return 0;
    }

    case DRM_IOCTL_V3D_GET_PARAM:
    {
        struct drm_v3d_get_param *p = arg;

        switch (p->param)
        {
        case DRM_V3D_PARAM_V3D_UIFCFG:      p->value = 0; break;
        case DRM_V3D_PARAM_V3D_HUB_IDENT1:  p->value = sd->hub_ident[1]; break;
        case DRM_V3D_PARAM_V3D_HUB_IDENT2:  p->value = sd->hub_ident[2]; break;
        case DRM_V3D_PARAM_V3D_HUB_IDENT3:  p->value = sd->hub_ident[3]; break;
        case DRM_V3D_PARAM_V3D_CORE0_IDENT0: p->value = sd->core_ident[0]; break;
        case DRM_V3D_PARAM_V3D_CORE0_IDENT1: p->value = sd->core_ident[1]; break;
        case DRM_V3D_PARAM_V3D_CORE0_IDENT2: p->value = sd->core_ident[2]; break;
        case DRM_V3D_PARAM_SUPPORTS_TFU:    p->value = 0; break;
        case DRM_V3D_PARAM_SUPPORTS_CSD:    p->value = 0; break;
        default:                            p->value = 0; break;
        }
        return 0;
    }

    case DRM_IOCTL_V3D_SUBMIT_TFU:
    case DRM_IOCTL_V3D_SUBMIT_CSD:
        /* Answered unsupported through GET_PARAM, so these are never
         * reached; refuse rather than pretend. */
        return -1;

    default:
        D(bug("[V3D] unhandled ioctl 0x%lx\n", request));
        return -1;
    }
}

/*
 * mmap/munmap as Mesa's bufmgr calls them: MMAP_BO already answered with
 * the buffer's identity-mapped uncached address in the offset field, so
 * mapping is handing that back and unmapping is nothing.
 */
void *mmap(void *addr, unsigned long length, int prot, int flags, int fd,
           long offset)
{
    (void)addr; (void)length; (void)prot; (void)flags; (void)fd;
    return (void *)(IPTR)offset;
}

int munmap(void *addr, unsigned long length)
{
    (void)addr; (void)length;
    return 0;
}

/*
 * The CLIF dumper prints control lists for V3D_DEBUG=cl sessions, through
 * a decoder that wants the packet XML embedded in a generated header. Not
 * worth carrying for a debug aid: a NULL from init makes every caller
 * skip its dump.
 */
struct clif_dump *clif_dump_init(const void *devinfo, void *output,
                                 int pretty)
{
    (void)devinfo; (void)output; (void)pretty;
    return NULL;
}

void clif_dump(struct clif_dump *clif, const void *submit)
{
    (void)clif; (void)submit;
}

void clif_dump_add_bo(struct clif_dump *clif, const char *name,
                      uint32_t offset, uint32_t size, void *vaddr)
{
    (void)clif; (void)name; (void)offset; (void)size; (void)vaddr;
}

void clif_dump_destroy(struct clif_dump *clif)
{
    (void)clif;
}

/* Not in AROS's stdc. The builtin compiles to rbit+clz on clang; gcc may
 * lower it to a call to ffs itself, which this would then be. */
int ffs(int i)
{
    return __builtin_ffs(i);
}

/*
 * Stubs for paths the screen configuration keeps closed: renderonly is
 * only reached with a renderonly context (ours is NULL), the driconf
 * queries only with an option cache (also NULL).
 */
struct renderonly *renderonly_dup(const struct renderonly *ro)
{
    (void)ro;
    return NULL;
}

struct renderonly_scanout *
renderonly_create_gpu_import_for_resource(struct pipe_resource *rsc,
                                          struct renderonly *ro,
                                          struct winsys_handle *out_handle)
{
    (void)rsc; (void)ro; (void)out_handle;
    return NULL;
}

void renderonly_scanout_destroy(struct renderonly_scanout *scanout,
                                struct renderonly *ro)
{
    (void)scanout; (void)ro;
}

unsigned char driCheckOption(const void *cache, const char *name, int type)
{
    (void)cache; (void)name; (void)type;
    return 0;
}

unsigned char driQueryOptionb(const void *cache, const char *name)
{
    (void)cache; (void)name;
    return 0;
}
