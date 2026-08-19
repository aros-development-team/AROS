/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    VideoCore VI (V3D 4.2) GPU — Hardware initialization and job submission
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <aros/macros.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "v3d_intern.h"

/* Register access */
static inline ULONG v3d_hub_rd(struct V3DData *sd, ULONG off)
{
    return AROS_LE2LONG(*(volatile ULONG *)(sd->hub_base + off));
}

static inline void v3d_hub_wr(struct V3DData *sd, ULONG off, ULONG val)
{
    *(volatile ULONG *)(sd->hub_base + off) = AROS_LONG2LE(val);
}

static inline ULONG v3d_core_rd(struct V3DData *sd, ULONG off)
{
    return AROS_LE2LONG(*(volatile ULONG *)(sd->core0_base + off));
}

static inline void v3d_core_wr(struct V3DData *sd, ULONG off, ULONG val)
{
    *(volatile ULONG *)(sd->core0_base + off) = AROS_LONG2LE(val);
}

/*
 * Power on the V3D block via VideoCore mailbox.
 */
static BOOL v3d_power_on(struct V3DData *sd)
{
    /* The V3D power domain should already be enabled by the firmware
     * if the GPU is in use for the framebuffer. If not, we'd need
     * to send a mailbox message to power it on.
     * For now, assume it's powered (firmware enables it at boot). */
    D(bug("[V3D] Assuming V3D is powered on by firmware\n"));
    return TRUE;
}

/*
 * Initialize V3D hardware — identify the GPU and verify it's alive.
 */
BOOL v3d_hw_init(struct V3DData *sd)
{
    ULONG ident0, ident1, ident2;

    sd->hub_base = V3D_HUB_BASE;
    sd->core0_base = V3D_CORE0_BASE;

    /* Power on */
    if (!v3d_power_on(sd))
        return FALSE;

    /* Read identification registers */
    ident0 = v3d_hub_rd(sd, V3D_HUB_IDENT0);
    ident1 = v3d_hub_rd(sd, V3D_HUB_IDENT1);
    ident2 = v3d_hub_rd(sd, V3D_HUB_IDENT2);

    sd->ver = (ident0 >> V3D_IDENT0_VER_SHIFT) & 0xFF;

    if (sd->ver == 0 || sd->ver == 0xFF) {
        D(bug("[V3D] V3D not responding (ident0=0x%08lx) — not powered?\n", ident0));
        return FALSE;
    }

    /* Extract core info from ident1 */
    sd->nslc = (ident1 >> 4) & 0xF;
    sd->qpus_per_slice = (ident1 >> 8) & 0xF;

    D(bug("[V3D] V3D %ld.%ld identified: %ld slices, %ld QPUs/slice\n",
          sd->ver / 10, sd->ver % 10, sd->nslc, sd->qpus_per_slice));
    D(bug("[V3D] IDENT0=0x%08lx IDENT1=0x%08lx IDENT2=0x%08lx\n",
          ident0, ident1, ident2));

    /* Mask all interrupts initially */
    v3d_hub_wr(sd, V3D_HUB_INT_MSK_SET, 0xFFFFFFFF);
    v3d_hub_wr(sd, V3D_HUB_INT_CLR, 0xFFFFFFFF);

    /* Initialize buffer and job lists */
    NEWLIST(&sd->bo_list);
    NEWLIST(&sd->job_list);
    InitSemaphore(&sd->bo_lock);
    InitSemaphore(&sd->job_lock);

    sd->powered = TRUE;

    return TRUE;
}

void v3d_hw_shutdown(struct V3DData *sd)
{
    /* Mask all interrupts */
    v3d_hub_wr(sd, V3D_HUB_INT_MSK_SET, 0xFFFFFFFF);
    sd->powered = FALSE;
}

/*
 * Allocate a GPU buffer object (physically contiguous).
 * The V3D GPU on BCM2711 without IOMMU sees physical addresses directly.
 */
struct V3DBO *v3d_aros_bo_alloc(struct V3DData *sd, ULONG size)
{
    struct V3DBO *bo;

    bo = AllocVec(sizeof(struct V3DBO), MEMF_CLEAR | MEMF_PUBLIC);
    if (!bo)
        return NULL;

    /* Allocate physically contiguous memory in the low 1GB
     * (V3D can address full 4GB but we keep it simple) */
    bo->vaddr = AllocVec(size, MEMF_CLEAR | MEMF_PUBLIC | MEMF_31BIT);
    if (!bo->vaddr) {
        FreeVec(bo);
        return NULL;
    }

    /* On AROS without MMU for userspace, virtual = physical */
    bo->paddr = (ULONG)(IPTR)bo->vaddr;
    bo->size = size;
    bo->refcount = 1;

    ObtainSemaphore(&sd->bo_lock);
    AddTail((struct List *)&sd->bo_list, (struct Node *)bo);
    ReleaseSemaphore(&sd->bo_lock);

    return bo;
}

void v3d_aros_bo_free(struct V3DData *sd, struct V3DBO *bo)
{
    if (--bo->refcount > 0)
        return;

    ObtainSemaphore(&sd->bo_lock);
    Remove((struct Node *)bo);
    ReleaseSemaphore(&sd->bo_lock);

    FreeVec(bo->vaddr);
    FreeVec(bo);
}

/*
 * Submit a control list to the V3D CLE (Control List Executor).
 *
 * Thread 0 = Binning (geometry processing)
 * Thread 1 = Rendering (fragment processing)
 *
 * The CLE starts executing when we write the end address.
 */
BOOL v3d_submit_cl(struct V3DData *sd, ULONG start, ULONG end, BOOL is_render)
{
    if (!sd->powered)
        return FALSE;

    /* Flush data cache so GPU sees the control list */
    CacheClearE((APTR)(IPTR)start, end - start, CACRF_ClearD);

    if (is_render) {
        /* Thread 1: Rendering */
        v3d_core_wr(sd, V3D_CLE_CT1CA, start);
        v3d_core_wr(sd, V3D_CLE_CT1EA, end);
    } else {
        /* Thread 0: Binning */
        v3d_core_wr(sd, V3D_CLE_CT0CA, start);
        v3d_core_wr(sd, V3D_CLE_CT0EA, end);
    }

    return TRUE;
}

/*
 * Wait for both CLE threads to become idle.
 */
void v3d_wait_idle(struct V3DData *sd)
{
    int tries = 1000000;

    while (--tries) {
        ULONG ct0cs = v3d_core_rd(sd, V3D_CLE_CT0CS);
        ULONG ct1cs = v3d_core_rd(sd, V3D_CLE_CT1CS);

        if (!(ct0cs & V3D_CLE_CTCS_RUN) && !(ct1cs & V3D_CLE_CTCS_RUN))
            return;
    }

    D(bug("[V3D] WARNING: Timeout waiting for CLE idle\n"));
}
