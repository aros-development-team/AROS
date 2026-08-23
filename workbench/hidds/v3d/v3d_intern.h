/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D 4.2) GPU driver internals.

    The runtime layer follows vc4gallium, which is hardware-proven on the
    VideoCore IV: buffer objects live in firmware-managed GPU memory
    reached through the uncached bus alias (no cache maintenance by
    construction), handles come from a rotating table that reuses freed
    slots, and the block is powered up through the mailbox before any
    register is read - an unpowered domain answers MMIO with an SError.
*/

#ifndef V3D_INTERN_H
#define V3D_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <oop/oop.h>
#include <hidd/gallium.h>

/*
 * Register blocks. The hub sits at the same peripheral offset the
 * VideoCore IV V3D did (bus 0x7ec00000), with core 0 just above it -
 * which is also why vc4gallium must never probe here: it would read
 * these registers and take them for a V3D 2.x.
 */
extern IPTR __arm_periiobase;
#define ARM_PERIIOBASE      __arm_periiobase
#define V3D_HUB_OFFSET      0xc00000
#define V3D_CORE0_OFFSET    0xc04000

/* Hub registers */
#define V3D_HUB_AXICFG      0x0000
#define V3D_HUB_UIFCFG      0x0004
#define V3D_HUB_IDENT0      0x0008
#define V3D_HUB_IDENT1      0x000c
#define V3D_HUB_IDENT2      0x0010
#define V3D_HUB_IDENT3      0x0014
#define V3D_HUB_INT_STS     0x0050
#define V3D_HUB_INT_SET     0x0054
#define V3D_HUB_INT_CLR     0x0058
#define V3D_HUB_INT_MSK_STS 0x005c
#define V3D_HUB_INT_MSK_SET 0x0060
#define V3D_HUB_INT_MSK_CLR 0x0064

/* Core registers (from V3D_CORE0_OFFSET) */
#define V3D_CTL_IDENT0      0x0000
#define V3D_CTL_IDENT1      0x0004
#define V3D_CTL_IDENT2      0x0008
#define V3D_CTL_MISCCFG     0x0018

/*
 * Cache control. The probe corroborates the layout: +020 read 1 with the
 * L2 enabled, and +034/+038 are the only fully writable neighbours here,
 * +038 resetting to all-ones - a flush range's start and end. The GPU's
 * read caches have to be invalidated around a job or it works from stale
 * memory, and the parts of a frame that survive are then whichever tiles
 * happened to miss.
 */
#define V3D_CTL_L2CACTL     0x0020
#define V3D_CTL_SLCACTL     0x0024
#define V3D_CTL_L2TCACTL    0x0030
#define V3D_CTL_L2TFLSTA    0x0034
#define V3D_CTL_L2TFLEND    0x0038
#define V3D_L2CACTL_ENABLE  (1 << 0)
#define V3D_L2CACTL_CLEAR   (1 << 2)
#define V3D_L2TCACTL_FLUSH  (1 << 0)
#define V3D_CTL_INT_STS     0x0050
#define V3D_CTL_INT_SET     0x0054
#define V3D_CTL_INT_CLR     0x0058
#define V3D_CTL_INT_MSK_STS 0x005c
#define V3D_CTL_INT_MSK_SET 0x0060
#define V3D_CTL_INT_MSK_CLR 0x0064

#define V3D_CLE_CT0CS       0x0100
#define V3D_CLE_CT1CS       0x0104
#define V3D_CLE_CT0EA       0x0108
#define V3D_CLE_CT1EA       0x010c
#define V3D_CLE_CT0CA       0x0110
#define V3D_CLE_CT1CA       0x0114
/*
 * Queue registers, located by probing the live core rather than guessed:
 * writing to the VideoCore IV offsets read back as zero while IDENT0 read
 * correctly from the same base. Four consecutive writable words at +160
 * with the masked tile-alloc pair right behind them, and the read-only
 * current-address registers at +110, identify them unambiguously - and
 * note they interleave by thread rather than pairing per queue.
 */
#define V3D_CLE_CT0QTS      0x015c
/* Without this bit the tile-state address is DISABLED and the binner
 * initialises the per-tile state wherever the internal default points -
 * measured on a Pi 400 as a sequential-integer spray over low ARM RAM
 * (exec's LVO table, SysBase's lists, the boot page tables), i.e. the
 * fullscreen "random" kernel crashes. The address field is simply not
 * honoured without it. */
#define V3D_CLE_CT0QTS_ENABLE (1UL << 31)
#define V3D_CLE_CT0QBA      0x0160
#define V3D_CLE_CT1QBA      0x0164
#define V3D_CLE_CT0QEA      0x0168
#define V3D_CLE_CT1QEA      0x016c
#define V3D_CLE_CT0QMA      0x0170  /* binner tile alloc: address, then */
#define V3D_CLE_CT0QMS      0x0174  /* size - required from 4.1 on */
#define V3D_CLE_CTCS_RUN    (1 << 5)

/*
 * PTB binner overflow (bare register ABI, same block layout as the
 * VideoCore IV's). If BPOA/BPOS are not programmed, a binner that
 * exhausts its tile allocation spills tile lists to whatever stale
 * address sits there - measured on a Pi 400 as GPU writes into LOW ARM
 * RAM: armstub, the live boot page tables (mapped addresses suddenly
 * translation-fault) and exec structures (the next IRQ jumps through a
 * smashed pointer). Windowed scenes never overflowed, which is why only
 * fullscreen crashed.
 */
#define V3D_PTB_BPCA        0x0300  /* current overflow address */
#define V3D_PTB_BPCS        0x0304  /* remaining overflow size */
#define V3D_PTB_BPOA        0x0308  /* next overflow supply: address */
#define V3D_PTB_BPOS        0x030c  /* next overflow supply: size */

#define V3D_OVERFLOW_SIZE   (4 << 20)

/* PTB quirk landing zone: real memory mapped at GPU VA 0, covering the
 * fixed low band the binner streams to regardless of programming (see
 * v3d_hw.c). The band scales with the tile count (~256 bytes/tile
 * measured: 320 tiles stayed under 128KB, 600 tiles at 1920x1280 hit
 * ~148KB and starved the renderer when the zone clipped it). 1MB
 * covers 4K displays with margin; report_once still watches for
 * violations if even that is outgrown. */
#define V3D_PTB_QUIRK_SIZE  (1 << 20)

/*
 * Core interrupt bits, both measured rather than assumed: bit 1 comes up
 * when a binner job completes and bit 0 when a render does - a bin-only
 * wait sees 0x2, and 0x3 once the render has run too. Guessing them in
 * the obvious order put render completion at bit 2, where nothing ever
 * arrived, and every frame waited out its timeout with both threads
 * sitting idle at their end addresses.
 *
 * Which bit carries the binner's request for more tile memory is NOT
 * known - nothing here has been seen to ask for it yet.
 */
#define V3D_INT_FRDONE      (1 << 0)
#define V3D_INT_FLDONE      (1 << 1)

/*
 * MMU (hub side; bare register ABI, every offset and bit below confirmed
 * by readback on hardware). Enabled with an identity map of the firmware
 * heap ONLY: every GPU access is translated, so a stray write (the
 * low-RAM sprays that ate exec's LVO tables, the boot page tables and the
 * IRQ lists at fullscreen sizes) faults into the scratch page and latches
 * the address in VIO_ADDR instead of corrupting the kernel.
 */
#define V3D_MMUC_CONTROL    0x1000
#define V3D_MMUC_CONTROL_FLUSHING   (1 << 2)
#define V3D_MMUC_CONTROL_FLUSH      (1 << 1)
#define V3D_MMUC_CONTROL_ENABLE     (1 << 0)

#define V3D_MMU_CTL         0x1200
#define V3D_MMU_CTL_CAP_EXCEEDED        (1 << 27)
#define V3D_MMU_CTL_CAP_EXCEEDED_ABORT  (1 << 26)
#define V3D_MMU_CTL_CAP_EXCEEDED_INT    (1 << 25)
#define V3D_MMU_CTL_PT_INVALID          (1 << 20)
#define V3D_MMU_CTL_PT_INVALID_ABORT    (1 << 19)
#define V3D_MMU_CTL_PT_INVALID_INT      (1 << 18)
#define V3D_MMU_CTL_PT_INVALID_ENABLE   (1 << 16)
#define V3D_MMU_CTL_WRITE_VIOLATION     (1 << 12)
#define V3D_MMU_CTL_WRITE_VIOLATION_ABORT (1 << 11)
#define V3D_MMU_CTL_WRITE_VIOLATION_INT (1 << 10)
#define V3D_MMU_CTL_TLB_CLEAR           (1 << 2)
#define V3D_MMU_CTL_ENABLE              (1 << 0)

#define V3D_MMU_PT_PA_BASE  0x1204
#define V3D_MMU_VIO_ID      0x122c
#define V3D_MMU_ILLEGAL_ADDR 0x1230     /* scratch page | ENABLE(1<<31) */
#define V3D_MMU_ILLEGAL_ADDR_ENABLE (1UL << 31)
#define V3D_MMU_VIO_ADDR    0x1234
#define V3D_MMU_DEBUG_INFO  0x1238

/* 32-bit PTEs, one per 4KB page of GPU VA; pfn in the low bits */
#define V3D_PTE_VALID       (1UL << 28)
#define V3D_PTE_WRITEABLE   (1UL << 29)

#define V3D_MMU_PT_SIZE     (4 << 20)   /* 1M PTEs cover 4GB of VA */

#define V3D_IRQ             (32 + 74)   /* GIC SPI 74, from the device tree */

/*
 * Power and reset. The device tree hangs the BCM2711's V3D off the
 * classic PM block (power-domains = <&pm 1>, resets = <&pm 0>), not the
 * firmware: the mailbox power tags answer "on" without the block ever
 * waking, and every register then reads the bus poison 0xdeadbeef. The
 * PM block's third register window on the 2711 is a new ASB instance
 * next to V3D itself, whose bridges have to be unstalled after power-up.
 * Register and bit meanings are the BCM2835 PM block's, unchanged.
 */
#define V3D_PM_OFFSET       0x100000    /* /soc, so periiobase-relative */
#define V3D_PM_GRAFX        0x10c
#define V3D_PM_PASSWORD     0x5a000000
#define V3D_PM_POWUP        (1 << 0)
#define V3D_PM_POWOK        (1 << 1)
#define V3D_PM_ISPOW        (1 << 2)
#define V3D_PM_MEMREP       (1 << 3)
#define V3D_PM_MRDONE       (1 << 4)
#define V3D_PM_ISFUNC       (1 << 5)
#define V3D_PM_V3DRSTN      (1 << 6)

#define V3D_ASB_OFFSET      0xc11000    /* the 2711-only instance */
#define V3D_ASB_V3D_S_CTRL  0x08
#define V3D_ASB_V3D_M_CTRL  0x0c
#define V3D_ASB_REQ_STOP    (1 << 0)
#define V3D_ASB_ACK         (1 << 1)

#define V3D_CLK_ID          5           /* firmware clock, as on the Pi 3 */

/* BCM system timer free-running counter, 1 MHz: wall-clock for the GPU
 * waits, so their budgets don't scale with the CPU clock. */
#define V3D_SYSTIMER_CLO    (ARM_PERIIOBASE + 0x3004)

/* vcgfx bitmap interface, mirrored from vcgfx_bitmap.h */
#define IID_Hidd_BitMap_VideoCore4  "hidd.bitmap.bcmvc4"
#define aoVCGfxBM_Drawable      0   /* [G] front page phys addr */
#define aoVCGfxBM_BackDrawable  1   /* [G] back page phys, 0 = no flip */
#define aoVCGfxBM_Flip          2   /* [S] TRUE = flip front/back */
#define aoVCGfxBM_Overlay       3   /* [GS] overlay descriptor */

/* Overlay descriptor (mirrored from vcgfx_bitmap.h, like the ids above):
 * a 32bpp plane the HVS scans straight from GPU memory, composited over
 * the framebuffer. x/y are fb coordinates. */
struct vc4gfx_overlay
{
    ULONG ovl_Phys;                 /* ARM phys of the pixel data */
    ULONG ovl_Pitch;                /* bytes per row */
    ULONG ovl_Width, ovl_Height;    /* source pixels */
    LONG  ovl_X, ovl_Y;             /* position in fb coordinates */
    ULONG ovl_DestW, ovl_DestH;     /* on-screen size; 0 (or == source)
                                     * = unscaled, larger = HVS upscale */
};

/*
 * GPU wait tiers (vc4gallium's proven values): tight spin for µs-precise
 * completion of short jobs, then ~1 ms timer naps so input and other
 * tasks keep running while the GPU renders, up to the hang timeout.
 */
#define V3D_GPUWAIT_SPIN_US     2000
#define V3D_GPUWAIT_NAP_US      1000
#define V3D_GPUWAIT_TIMEOUT_US  1000000

/*
 * The CPU reaches a BO at its locked physical address (identity through
 * the uncached VideoCore window). The GPU gets a LOW VIRTUAL address
 * through the V3D MMU instead: the hardware is only ever exercised with
 * small VAs in practice, and feeding it identity-mapped ~1GB physicals
 * made the PTB compute stray writes into low RAM (an internal
 * truncation/banking path nothing else exercises). paddr & 0x1FFFFFFF is
 * collision-free for a <=512MB firmware heap and needs no allocator
 * state.
 */
#define V3D_GPU_ADDR(phys)  ((ULONG)(phys) & 0x3fffffff)
#define V3D_GPU_VA(paddr)   ((ULONG)(paddr) & 0x1fffffff)

/*
 * A buffer object. gpu_handle is the firmware allocation (ALLOCMEM), and
 * the memory stays locked for the BO's whole life: paddr doubles as the
 * CPU mapping through the identity-mapped uncached VideoCore region, so
 * nothing is ever flushed or invalidated for the GPU's benefit.
 */
struct V3DBO
{
    ULONG   gpu_handle;     /* firmware handle, 0 = slot free/external */
    ULONG   paddr;          /* locked address, alias bits masked (CPU) */
    ULONG   gpu_va;         /* what the GPU sees, via the V3D MMU */
    ULONG   size;
    ULONG   refcount;
    ULONG   external;       /* wraps memory we do not own (a scanout
                             * page): unmap on close, never FREEMEM */
};

#define V3D_MAX_BOS     1024

struct V3DData
{
    OOP_Class       *galliumclass;

    OOP_AttrBase    hiddGalliumAB;
    OOP_AttrBase    hiddAttrBase;
    OOP_AttrBase    hiddBitMapAB;
    OOP_AttrBase    hiddVCGfxBMAB;

    APTR            mbox_base;
    APTR            mbox_msg_raw;
    volatile ULONG  *mbox_msg;
    struct SignalSemaphore mbox_lock;

    IPTR            hub_base;
    IPTR            core0_base;

    /* Cached at probe; DRM_V3D_GET_PARAM serves these without MMIO. */
    ULONG           hub_ident[4];
    ULONG           core_ident[3];
    ULONG           ver;            /* 42 on a BCM2711 */

    /*
     * The pipeline (all under job_lock). One frame consists of a bin job
     * and a stashed render job; the render is kicked only when its bin
     * has FLUSHED (FLDONE) and the previous render has retired (FRDONE).
     * A new bin may start while the previous render still runs - that
     * overlap is the point of submitting asynchronously.
     */
    struct SignalSemaphore job_lock;
    ULONG           seqno;          /* last submission accepted */
    ULONG           finished_seqno; /* last submission fully retired */
    BOOL            bin_running;
    BOOL            render_running;
    ULONG           bin_seqno;
    ULONG           render_seqno;
    ULONG           pending_seqno;
    ULONG           pending_rcl_start;
    ULONG           pending_rcl_end;    /* 0 = no render stashed */

    /* End addresses of the last kicked jobs, for the stall dump */
    ULONG           bin_end;
    ULONG           render_end;

    /* Two binner overflow BOs, armed alternately per bin job: render N
     * may still read tile lists in one while bin N+1 arms the other.
     * Reuse is safe - bin N+2 cannot start until render N retired. */
    ULONG           overflow_handle[2];
    ULONG           overflow_paddr[2];
    ULONG           overflow_va[2];

    /* MMU page table + violation scratch page, firmware-allocated once */
    ULONG           mmu_pt_handle;
    ULONG           mmu_pt_paddr;
    ULONG           mmu_scratch_handle;
    ULONG           mmu_scratch_paddr;
    ULONG           ptb_quirk_handle;   /* landing zone at GPU VA 0 */
    ULONG           ptb_quirk_paddr;

    struct V3DBO    bo_table[V3D_MAX_BOS];
    ULONG           bo_next_handle;
    struct SignalSemaphore bo_lock;

    /* The framebuffer flip pages, published (under bo_lock) as the only
     * names DRM_IOCTL_GEM_OPEN accepts - the zero-copy fullscreen path
     * wraps them as BOs and renders straight into the back page. */
    ULONG           scanout_phys[2];
    ULONG           scanout_size;

    /* timer.device (UNIT_MICROHZ) for the wait loops' microsleeps. Only
     * io_Device/io_Unit are kept - each nap clones them into a stack
     * request, so any task may wait. */
    struct timerequest gpu_timer_template;
    BOOL            gpu_timer_ok;

    APTR            coreapi;        /* GalliumCoreAPI table from mesa3dgl */
    BOOL            powered;

    /* Live pipe screens (a GL app makes more than one: the capability
     * probe's, then the real one) - the BO sweep and state reset only
     * run when the LAST one goes. */
    LONG            screen_count;
    ULONG           recoveries;     /* hang-recovery fuse, per session */
};

struct IntHiddV3DBase
{
    struct Library  lib;
    struct V3DData  sd;
};

#define BASE(lib) ((struct IntHiddV3DBase *)(lib))
#define SD(cl)    (&BASE(cl->UserData)->sd)

/* v3d_init.c */
BOOL v3d_block_reset(void);

/* v3d_hw.c */
BOOL v3d_hw_init(struct V3DData *sd);
void v3d_hw_shutdown(struct V3DData *sd);
ULONG v3d_mmu_map(struct V3DData *sd, ULONG paddr, ULONG size);
void v3d_mmu_unmap(struct V3DData *sd, ULONG gpu_va, ULONG size);
BOOL v3d_submit_cl(struct V3DData *sd, ULONG bcl_start, ULONG bcl_end,
                   ULONG qma, ULONG qms, ULONG qts,
                   ULONG rcl_start, ULONG rcl_end);
void v3d_wait_idle(struct V3DData *sd);
void v3d_flush_caches(struct V3DData *sd);

/* v3d_drm_shim.c */
ULONG v3d_gpu_mem_alloc(struct V3DData *sd, ULONG size, ULONG align,
                        ULONG *out_paddr);
void v3d_gpu_mem_free(struct V3DData *sd, ULONG gpu_handle);
int v3d_ioctl_aros(struct V3DData *sd, unsigned long request, void *arg);
void v3d_release_all_bos(struct V3DData *sd);

#endif /* V3D_INTERN_H */
