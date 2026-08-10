/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    VC4 Gallium 3D HIDD - Internal definitions
*/

#ifndef _VC4GALLIUM_INTERN_H
#define _VC4GALLIUM_INTERN_H

#include <stdint.h>
#include <stdbool.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <devices/timer.h>
#include <oop/oop.h>
#include <hidd/gallium.h>

#include LC_LIBDEFS_FILE

#include "vc4_aros_bridge.h"

#define CLID_Hidd_Gallium_VC4   "hidd.gallium.vc4"

struct HiddGalliumVC4Data
{
    OOP_Object              *vc4_obj;
};

/* Buffer Object handle tracking. Entries are ~760 bytes (shader metadata
 * inline), so the table is ~760 KB of library-base memory at 1024. */
#define VC4_MAX_BOS     1024

/* Max texture samples and uniform address offsets per shader */
#define VC4_MAX_TEX_SAMPLES     32
#define VC4_MAX_UNIFORM_ADDR    16

/* Per-texture-sample metadata from QPU shader scan.
 * p_offset[i] is the byte offset into the uniform data stream where
 * texture parameter i (P0/P1/P2/P3) is located. ~0 = unused. */
struct vc4_texture_sample_info
{
    BOOL    is_direct;      /* UBO direct access mode */
    ULONG   p_offset[4];   /* Offsets to P0, P1, P2, P3 in uniform stream */
};

struct vc4_bo_entry
{
    APTR    vaddr;          /* CPU virtual address (= physical on RPi) */
    ULONG   bus_addr;       /* GPU bus address (0xC0000000 | phys) */
    ULONG   gpu_handle;     /* Mailbox memory handle for free */
    ULONG   size;
    ULONG   refcount;
    ULONG   seqno;          /* Last submission that referenced this BO (0 = never) */
    BOOL    is_shader;      /* Immutable shader BO */
    BOOL    external;       /* Wraps memory we don't own (scanout page) */
    BOOL    cpu_mapped;     /* MMAP_BO was called — CPU may hold dirty lines */
    UQUAD   tiling_modifier;/* DRM_FORMAT_MOD_* — round-tripped via set/get_tiling */
    /* Shader metadata (set by QPU scanner on CREATE_SHADER_BO) */
    ULONG   uniforms_size;          /* Bytes of uniform data GPU reads */
    ULONG   num_texture_samples;    /* Number of texture lookups */
    struct vc4_texture_sample_info texture_samples[VC4_MAX_TEX_SAMPLES];
    ULONG   num_uniform_addr_offsets;
    ULONG   uniform_addr_offsets[VC4_MAX_UNIFORM_ADDR]; /* Indices into uniform words */
};

/* V3D hardware state */
struct vc4_v3d_state
{
    volatile ULONG  *v3d_regs;      /* Mapped V3D register block */
    ULONG           ident0;         /* V3D_IDENT0 value */
    ULONG           ident1;         /* V3D_IDENT1 value */
    ULONG           ident2;         /* V3D_IDENT2 value */
    ULONG           v3d_ver;        /* V3D version from IDENT0 */
    volatile ULONG  seqno;          /* Monotonically increasing job seqno */
    volatile ULONG  finished_seqno; /* Last completed seqno */
    /* Frame counter tracking. BFC/RFC are 8-bit wrapping counters in V3D;
     * we accumulate the delta into 32-bit completed counts. finished_seqno
     * is driven by rfc_completed (render frame done = job done). */
    volatile ULONG  last_bfc;       /* Last raw BFC sample (low 8 bits) */
    volatile ULONG  last_rfc;       /* Last raw RFC sample (low 8 bits) */
    volatile ULONG  bfc_completed;  /* Bin frames completed since init */
    volatile ULONG  rfc_completed;  /* Render frames completed since init */
    BOOL            v3d_available;  /* TRUE if V3D block responds */
    /* IRQ diagnostics + handoff state. FLDONE handler kicks CT1 with the
     * addresses submit_cl stashed in pending_ct1{ca,ea}. */
    APTR            irq_handle;
    volatile ULONG  int_outomem;    /* Binner OUT_OF_MEMORY count */
    volatile ULONG  int_fldone;     /* Bin flush done count (for diff vs BFC) */
    volatile ULONG  int_frdone;     /* Render frame done count (for diff vs RFC) */
    /* IRQ-only counters (incremented solely in the ARM IRQ handler, never the
     * poll path) - lets us tell whether IRQ_VC_3D actually reaches the ARM. */
    volatile ULONG  irq_calls;      /* v3d_irq_handler entries                 */
    volatile ULONG  irq_fldone;     /* FLDONE seen at IRQ entry                */
    volatile ULONG  irq_frdone;     /* FRDONE seen at IRQ entry                */
    volatile ULONG  irq_outomem;    /* OUTOMEM seen at IRQ entry               */
    volatile ULONG  pending_ct1ca;  /* RCL start address, latched by submit_cl */
    volatile ULONG  pending_ct1ea;  /* RCL end address */
    volatile BOOL   pending_render; /* TRUE while waiting for FLDONE to kick CT1 */
    volatile ULONG  kick_count;     /* Total CT1 kicks actually written */
    /* Binner overspill pool, fed on-demand from the OUTOMEM IRQ. submit_cl
     * pre-allocates the BO (no alloc in IRQ context), stashes its bus addr/size
     * here and resets overflow_handed; the handler writes BPOA/BPOS once. */
    volatile ULONG  overflow_bus;   /* Bus addr of binner overspill BO */
    volatile ULONG  overflow_size;  /* Size of that BO */
    volatile ULONG  overflow_handed;/* Times handed this submission (storm guard) */
};

/* Per-pool-set binner overspill BO. 512 KB covers worst-case tile-state +
 * tile-alloc (~320 KB at 4096 tiles) plus per-bin 32 B CL overflow, keeping
 * the VideoCore footprint small (2 sets = 1 MB). */
#define VC4_BIN_OVERFLOW_SIZE   (512 * 1024)

/* DMA channel for the display blit is allocated from dma.resource with
 * DMACHF_TDMODE (2D stride mode needs a full engine). The shared control
 * block layout and bus-alias macro come from the same header. */
#include <hardware/bcm2708_dma.h>

/* Reusable per-frame GPU BO — grows but never shrinks */
struct vc4_frame_bo
{
    APTR    vaddr;
    ULONG   gpu_handle;
    ULONG   bus_addr;
    ULONG   size;           /* Current allocated size (0 = not allocated) */
};

/* Static data for the module */
struct vc4galliumstaticdata
{
    OOP_Class           *galliumclass;
    OOP_AttrBase        hiddGalliumAB;
    struct Library      *UtilityBase;
    struct Library      *MBoxBase;

    struct vc4_v3d_state    v3d;

    /* Serializes all GPU submission/completion waits (submit_cl / wait_seqno
     * / wait_idle) so V3D regs, job seqno, pool ping-pong and FLDONE->CT1
     * handoff state are never touched by two tasks at once. Nestable:
     * submit_cl holds it across its internal pool-reuse wait. */
    struct SignalSemaphore  render_lock;

    /* BO handle table */
    struct vc4_bo_entry     bo_table[VC4_MAX_BOS];
    ULONG                   bo_next_handle;
    struct SignalSemaphore  bo_lock;

    /* Firmware heap accounting: every ALLOCMEM/FREEMEM pair updates these
     * so an allocation failure can report what the driver itself holds
     * versus how big the VideoCore partition is (gpu_mem= in config.txt).
     * vcram_size 0 = not queried yet. */
    ULONG                   gpu_mem_bytes;
    ULONG                   gpu_mem_allocs;
    ULONG                   vcram_base;
    ULONG                   vcram_size;

    /* Mailbox message buffer (16-byte aligned) */
    APTR                    mbox_msg_raw;   /* Raw allocation for FreeMem */
    volatile ULONG          *mbox_msg;      /* 16-byte aligned pointer */
    struct SignalSemaphore  mbox_lock;

    /* Vblank-driven GPU wait. An INTB_VERTB server signals the one registered
     * waiter each vblank (~50 Hz) so the wait paths block instead of spinning,
     * freeing the CPU between vblanks. wait_gate serializes waiters (one slot
     * suffices); the server only does a lockless Signal() in IRQ context. */
    struct Interrupt        vblank_server;
    struct Task             *vblank_waiter;
    BYTE                    vblank_waiter_sig;
    BOOL                    vblank_added;
    struct SignalSemaphore  wait_gate;

    /* vc4gfx bitmap detection */
    OOP_AttrBase            hiddVC4GfxBMAB;     /* VideoCoreGfxBitMap attr base */
    OOP_AttrBase            hiddBitMapAB;       /* standard BitMap attr base */

    /* Rotating BO pools — three sets so the CPU can prepare frame N+1
     * while the GPU renders frame N and frame N-1's set drains. With
     * only two sets the submit of N blocked until N-2's render was done,
     * which re-serialized the pipelined present path. */
#define VC4_NUM_POOL_SETS   3
    struct {
        struct vc4_frame_bo tile;
        struct vc4_frame_bo exec;
        struct vc4_frame_bo rcl;
        struct vc4_frame_bo binoverflow;    /* Binner overspill pool (BPOA/BPOS) */
        ULONG               seqno;  /* Seqno of last job using this set (0 = idle) */
    } pool[VC4_NUM_POOL_SETS];
    ULONG                   pool_idx;   /* Current set index */

    /* Scratch for process_bin_cl's per-GL_SHADER_STATE flag list. Grown
     * on demand to bin_cl_size/5 (the max shader-state packets a CL can
     * hold); persistent because submit_cl is serialized under render_lock,
     * so a stack array would either overflow (up to ~7920 draws/job) or
     * a per-submit alloc would leak on submit_cl's many early returns. */
    ULONG                   *shader_state_scratch;
    ULONG                   shader_state_scratch_max;

    /* Scanout pages announced via the bridge's get_scanout entry. GEM_OPEN
     * consults this to wrap a page as an external BO (the only names it
     * accepts). Written under bo_lock. */
    ULONG                   scanout_phys[2];
    ULONG                   scanout_size;

    /* Pre-opened timer.device (UNIT_MICROHZ) for GPU-wait microsleeps —
     * io_Device/io_Unit are cloned into a stack request per use, so any
     * task can sleep on it. */
    struct timerequest      gpu_timer_template;
    BOOL                    gpu_timer_ok;
    /* DMA blit state */
    LONG                    dmaChannel;         /* From dma.resource, -1 = none */
    BOOL                    dmaBusy;
    APTR                    dmaStaging;         /* Staging buffer (grows) */
    ULONG                   dmaStagingSize;
    APTR                    dmaCBRaw;           /* CB chain raw alloc */
    ULONG                   dmaCBRawSize;
    /* BO handle whose pin is currently owned by the in-flight async DMA.
     * Released through vc4_aros_bo_unref_locked once vc4_aros_dma_wait_idle
     * observes completion — keeps the firmware allocation alive across
     * a Mesa-side GEM_CLOSE racing the display swap. 0 = none. */
    ULONG                   dma_pinned_handle;

    /* BO currently scanned out as the HVS overlay plane (windowed
     * zero-copy GL), pinned until replaced or cleared; the bitmap it
     * was shown on, for clear_overlay(NULL) at context teardown. */
    ULONG                   overlay_pinned_handle;
    OOP_Object             *overlay_bm;

    /* Module-internal dispatch struct; the driver's winsys shims
     * (aros_drm_shim.c) route drmIoctl/mmap through it. */
    struct vc4_aros_bridge  bridge;
    BOOL                    bridge_inited;

    /* mesa3dgl's GalliumCoreAPI table (aHidd_Gallium_CoreAPI attr at New).
     * The driver's Mesa-core trampolines bind to it at CreatePipeScreen. */
    APTR                    coreapi;
};

LIBBASETYPE
{
    struct Library              LibNode;
    struct vc4galliumstaticdata sd;
};

/* Per-frame timing. Flip to 0 to silence. Reads SYSTIMER_CLO (1 MHz).
 * Output format is one bug() line per measured stage, so a single grep
 * '[VC4Prof]' over a serial log gives a CSV of the bottleneck. */
#define VC4G_PROFILE 0          /* periodic frame-budget summary (1 line / 120 frames) */
#define VC4G_PROFILE_FRAME 0    /* per-frame submit_cl/display_blit dumps (serial-heavy) */

#if VC4G_PROFILE || VC4G_PROFILE_FRAME
#include <hardware/bcm2708.h>
#define VC4G_NOW_US() ((ULONG)AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO))
#else
#define VC4G_NOW_US() 0
#endif

#if VC4G_PROFILE
#define VC4G_PROF(...) bug(__VA_ARGS__)
#else
#define VC4G_PROF(...) do { } while (0)
#endif

#if VC4G_PROFILE_FRAME
#define VC4G_PROFF(...) bug(__VA_ARGS__)
#else
#define VC4G_PROFF(...) do { } while (0)
#endif

/* Driver entry points in aros_drm_vc4.c (the winsys/present half). */
struct pipe_screen;
struct RastPort;
struct pipe_screen *vc4_aros_create_screen(struct vc4_aros_bridge *bridge,
                                           APTR coreapi);
IPTR vc4_aros_display_rp(APTR resource, LONG srcx, LONG srcy,
                         struct RastPort *rp, LONG dstx, LONG dsty,
                         LONG width, LONG height);
BOOL aros_drm_release_bridge(void);

/* Drain any in-flight display-blit DMA. Defined in vc4_galliumclass.c. */
void vc4_aros_dma_wait_idle(struct vc4galliumstaticdata *sd);

/* Wait for all submitted V3D work and flush its L2. Defined in
 * vc4_galliumclass.c. */
void vc4_aros_wait_idle(struct vc4galliumstaticdata *sd);

/* Hang forensics (vc4_drm_aros.c): dump the most recent submission's
 * geometry + RCL head/tail + first binner sublist. */
void v3d_dump_last_submit(void);

/* GPU wait tiers, shared by v3d_wait_seqno and vc4_aros_wait_idle.
 * Tight spin gives µs-precise completion for short jobs; the nap window
 * polls on a ~1 ms grid while the CPU sleeps via timer.device (keeps
 * input/mouse responsive during the GPU render); anything longer falls
 * back to vblank blocking. */
#define VC4_GPUWAIT_SPIN_US     2000
#define VC4_GPUWAIT_NAP_US      1000
#define VC4_GPUWAIT_NAP_WINDOW  20000

/* ~1 ms scheduler-friendly sleep. Defined in vc4_galliumclass.c. */
void vc4_gpu_nap(struct vc4galliumstaticdata *sd, ULONG us);


/* GPU-wait blocking helpers (defined in vc4_init.c). vc4_wait_enter claims the
 * single vblank-waiter slot and returns an allocated signal bit (-1 if none/no
 * slot); the wait loops then block on Wait(1<<sig), woken each vblank by the
 * INTB_VERTB server. vc4_wait_leave releases the slot. */
BYTE vc4_wait_enter(struct vc4galliumstaticdata *sd);
void vc4_wait_leave(struct vc4galliumstaticdata *sd, BYTE sig);

/* Diagnostic clock logging (defined in vc4_init.c). */
void vc4_log_clocks(struct vc4galliumstaticdata *sd, const char *when);

#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)

#endif /* _VC4GALLIUM_INTERN_H */
