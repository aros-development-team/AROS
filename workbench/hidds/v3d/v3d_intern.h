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
#define V3D_CLE_CT0QBA      0x0160
#define V3D_CLE_CT1QBA      0x0164
#define V3D_CLE_CT0QEA      0x0168
#define V3D_CLE_CT1QEA      0x016c
#define V3D_CLE_CT0QMA      0x0170  /* binner tile alloc: address, then */
#define V3D_CLE_CT0QMS      0x0174  /* size - required from 4.1 on */
#define V3D_CLE_CTCS_RUN    (1 << 5)

/* MMU (hub side). Whether the firmware leaves it enabled decides how BO
 * addresses reach the GPU; the init dump answers that on real hardware. */
#define V3D_MMU_CTL         0x1200
#define V3D_MMU_CTL_ENABLE  (1 << 31)
#define V3D_MMU_PT_PA_BASE  0x1204
#define V3D_MMU_ILLEGAL_ADDR 0x1230

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

/*
 * How a BO's locked bus address is presented to the GPU. Phase B decides
 * this from the MMU dump: with the MMU off the masked physical address is
 * what the units consume, exactly as on the VideoCore IV.
 */
#define V3D_GPU_ADDR(phys)  ((ULONG)(phys) & 0x3fffffff)

/*
 * A buffer object. gpu_handle is the firmware allocation (ALLOCMEM), and
 * the memory stays locked for the BO's whole life: paddr doubles as the
 * CPU mapping through the identity-mapped uncached VideoCore region, so
 * nothing is ever flushed or invalidated for the GPU's benefit.
 */
struct V3DBO
{
    ULONG   gpu_handle;     /* firmware handle, 0 = slot free */
    ULONG   paddr;          /* locked address, alias bits masked */
    ULONG   size;
    ULONG   refcount;
};

#define V3D_MAX_BOS     1024

struct V3DData
{
    OOP_Class       *galliumclass;

    OOP_AttrBase    hiddGalliumAB;
    OOP_AttrBase    hiddAttrBase;

    struct Library  *UtilityBase;

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

    struct V3DBO    bo_table[V3D_MAX_BOS];
    ULONG           bo_next_handle;
    struct SignalSemaphore bo_lock;

    APTR            coreapi;        /* GalliumCoreAPI table from mesa3dgl */
    BOOL            powered;
};

struct IntHiddV3DBase
{
    struct Library  lib;
    struct V3DData  sd;
};

#define BASE(lib) ((struct IntHiddV3DBase *)(lib))
#define SD(cl)    (&BASE(cl->UserData)->sd)

/* v3d_hw.c */
BOOL v3d_hw_init(struct V3DData *sd);
void v3d_hw_shutdown(struct V3DData *sd);
BOOL v3d_submit_bin(struct V3DData *sd, ULONG start, ULONG end,
                    ULONG qma, ULONG qms, ULONG qts);
BOOL v3d_submit_render(struct V3DData *sd, ULONG start, ULONG end);
void v3d_wait_idle(struct V3DData *sd);

/* v3d_drm_shim.c */
ULONG v3d_gpu_mem_alloc(struct V3DData *sd, ULONG size, ULONG align,
                        ULONG *out_paddr);
void v3d_gpu_mem_free(struct V3DData *sd, ULONG gpu_handle);
int v3d_ioctl_aros(struct V3DData *sd, unsigned long request, void *arg);

#endif /* V3D_INTERN_H */
