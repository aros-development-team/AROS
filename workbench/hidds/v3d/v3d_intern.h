/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    VideoCore VI (V3D 4.2) GPU Internal Definitions
*/

#ifndef V3D_INTERN_H
#define V3D_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <oop/oop.h>
#include <hidd/gallium.h>

/*
 * VideoCore VI (V3D 4.2) GPU driver for BCM2711 (Raspberry Pi 4)
 *
 * Register blocks:
 *   Hub:   0xFEC00000 (V3D_HUB) — top-level control, MMU, cache
 *   Core0: 0xFEC04000 (V3D_CORE0) — rendering core
 *
 * The V3D GPU is a tile-based deferred renderer:
 *   1. Binning pass: geometry → per-tile command lists
 *   2. Rendering pass: per-tile rasterization + fragment shading
 *
 * Jobs are submitted by writing Control List addresses to the
 * CLE (Control List Executor) registers.
 */

/* V3D Hub register base (ARM physical) */
#define V3D_HUB_BASE        0xFEC00000
#define V3D_CORE0_BASE      0xFEC04000

/* Hub registers */
#define V3D_HUB_IDENT0      0x0000  /* V3D Identification 0 */
#define V3D_HUB_IDENT1      0x0004  /* V3D Identification 1 */
#define V3D_HUB_IDENT2      0x0008  /* V3D Identification 2 */
#define V3D_HUB_IDENT3      0x000C  /* V3D Identification 3 */
#define V3D_HUB_INT_STS     0x0050  /* Hub interrupt status */
#define V3D_HUB_INT_SET     0x0054  /* Hub interrupt set */
#define V3D_HUB_INT_CLR     0x0058  /* Hub interrupt clear */
#define V3D_HUB_INT_MSK_STS 0x005C  /* Hub interrupt mask status */
#define V3D_HUB_INT_MSK_SET 0x0060  /* Hub interrupt mask set */
#define V3D_HUB_INT_MSK_CLR 0x0064  /* Hub interrupt mask clear */

/* Core registers (offset from V3D_CORE0_BASE) */
#define V3D_CTL_IDENT0      0x0000  /* Core identification */
#define V3D_CTL_IDENT1      0x0004
#define V3D_CTL_IDENT2      0x0008
#define V3D_CLE_CT0CS       0x0100  /* Control List Executor Thread 0 Control/Status */
#define V3D_CLE_CT0CA       0x0104  /* Thread 0 Current Address */
#define V3D_CLE_CT0EA       0x0108  /* Thread 0 End Address */
#define V3D_CLE_CT1CS       0x010C  /* Thread 1 Control/Status */
#define V3D_CLE_CT1CA       0x0110  /* Thread 1 Current Address */
#define V3D_CLE_CT1EA       0x0114  /* Thread 1 End Address */
#define V3D_CLE_CT0QTS      0x0118  /* Thread 0 Queue Total Submitted */
#define V3D_CLE_CT0QBA      0x011C  /* Thread 0 Queue Branch Address */
#define V3D_CLE_CT1QTS      0x0120  /* Thread 1 Queue Total Submitted */
#define V3D_CLE_CT1QBA      0x0124  /* Thread 1 Queue Branch Address */

/* CLE status bits */
#define V3D_CLE_CTCS_RUN    (1 << 5)  /* CLE is running */

/* V3D_CTL_IDENT0 fields */
#define V3D_IDENT0_VER_SHIFT 24

/* GCA (GPU Cache Allocator) */
#define V3D_GCA_CACHE_CTRL  0x0C00
#define V3D_GCA_SAFE_SHUTDOWN (1 << 19)
#define V3D_GCA_SAFE_SHUTDOWN_ACK (1 << 20)

/* MMU registers */
#define V3D_MMU_CTL         0x1000
#define V3D_MMU_PT_PA_BASE  0x1004
#define V3D_MMU_ILLEGAL_ADDR 0x1030

/* IRQ number (GIC SPI 74) */
#define V3D_IRQ             (32 + 74)

/* Power management via VideoCore mailbox */
#define VCPOWER_V3D         10  /* V3D power domain ID for mailbox */

/*
 * Buffer Object — represents a GPU-accessible memory allocation.
 * On RPi4 without IOMMU, we use physically-contiguous memory
 * (the GPU sees physical addresses directly).
 */
struct V3DBO {
    struct MinNode  node;
    APTR            vaddr;      /* CPU virtual address */
    ULONG           paddr;      /* Physical address (for GPU) */
    ULONG           size;
    ULONG           refcount;
};

/*
 * Job — a binning or rendering submission to the GPU.
 */
struct V3DJob {
    struct MinNode  node;
    struct V3DBO    *start;     /* Control list BO */
    ULONG           cl_start;   /* CL start address (physical) */
    ULONG           cl_end;     /* CL end address (physical) */
    BOOL            is_render;  /* FALSE=bin, TRUE=render */
    volatile BOOL   done;       /* Set by IRQ handler */
};

/*
 * Driver static data
 */
struct V3DData {
    OOP_Class       *galliumclass;  /* Our Gallium class pointer */

    OOP_AttrBase    hiddGalliumAB;
    OOP_AttrBase    hiddAttrBase;

    struct Library  *CyberGfxBase;
    struct Library  *UtilityBase;

    IPTR            hub_base;       /* V3D Hub MMIO */
    IPTR            core0_base;     /* V3D Core0 MMIO */

    ULONG           ver;            /* V3D version (42 for RPi4) */
    ULONG           nslc;           /* Number of slices */
    ULONG           qpus_per_slice; /* QPUs per slice */

    /* Buffer management */
    struct MinList  bo_list;
    struct SignalSemaphore bo_lock;

    /* Job queue */
    struct MinList  job_list;
    struct SignalSemaphore job_lock;

    /* IRQ */
    APTR            irq_handle;
    struct Task     *irq_task;
    ULONG           irq_signal;

    BOOL            powered;
};

struct IntHiddV3DBase {
    struct Library  lib;
    struct V3DData  sd;
};

#define BASE(lib) ((struct IntHiddV3DBase *)(lib))
#define SD(cl)    (&BASE(cl->UserData)->sd)

/* Hardware functions */
BOOL v3d_hw_init(struct V3DData *sd);
void v3d_hw_shutdown(struct V3DData *sd);
struct V3DBO *v3d_aros_bo_alloc(struct V3DData *sd, ULONG size);
void v3d_aros_bo_free(struct V3DData *sd, struct V3DBO *bo);
BOOL v3d_submit_cl(struct V3DData *sd, ULONG start, ULONG end, BOOL is_render);
void v3d_wait_idle(struct V3DData *sd);

#endif /* V3D_INTERN_H */
