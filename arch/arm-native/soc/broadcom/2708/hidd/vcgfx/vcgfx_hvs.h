#ifndef _VIDEOCOREGFX_HVS_H
#define _VIDEOCOREGFX_HVS_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 HVS (Hardware Video Scaler) and PixelValve
          register description.

    Sources: block base addresses and interrupt numbers are from the
    public BCM283x device tree (hvs@7e400000, pixelvalve@7e206000/
    7e207000/7e807000); register offsets, display-list word layout and
    pixel-format codes are community-documented VideoCore IV hardware
    facts. Written from scratch for AROS (APL) — no GPL code derived.

    The display-list layout below was VALIDATED against a live
    firmware-built list on a real Pi 3B+ (2026-07: 720p50 with 48px
    overscan, 1184x624 XRGB fb + 32x32 ARGB firmware cursor plane).
    Firmware drives HDMI on HVS channel 1 feeding PV2.
*/

#include <exec/types.h>

/* ------------------------------------------------------------------ */
/* HVS: bus 0x7E400000                                                 */
/* ------------------------------------------------------------------ */

#define VC4_HVS_BASE            (ARM_PERIIOBASE + 0x400000)

/* Global registers (byte offsets) */
#define HVS_DISPCTRL            0x0000  /* bit 31 = master enable */
#define HVS_DISPSTAT            0x0004
#define HVS_ID                  0x0008  /* reads 0x64647276 on real HW, 0 on QEMU */
#define HVS_DISPECTRL           0x000c
#define HVS_DISPPROF            0x0010
#define HVS_DISPDITHER          0x0014
#define HVS_DISPEOLN            0x0018

/* Per-channel display list head: word index into the display list RAM.
 * Writing DISPLISTx arms the next list; the HVS latches it at frame
 * start, which is the atomic flip mechanism. DISPLACTx reads back the
 * list the channel is currently scanning out. The firmware double-
 * buffers: every display update builds a fresh list elsewhere in dlist
 * RAM and repoints DISPLISTx. */
#define HVS_DISPLIST(ch)        (0x0020 + 4 * (ch))
#define HVS_DISPLSTAT           0x002c
#define HVS_DISPLACT(ch)        (0x0030 + 4 * (ch))

/* Per-channel control block, stride 0x10.
 * DISPCTRLX = ENABLE | output_width << 12 | output_height
 * (validated: 0x805002d0 = enabled, 1280x720). */
#define HVS_DISPCTRLX(ch)       (0x0040 + 0x10 * (ch))
#define HVS_DISPCTRLX_ENABLE    (1UL << 31)
#define HVS_DISPCTRLX_W_SHIFT   12
#define HVS_DISPCTRLX_H_SHIFT   0
#define HVS_DISPBKGNDX(ch)      (0x0044 + 0x10 * (ch))  /* background fill */
#define HVS_DISPSTATX(ch)       (0x0048 + 0x10 * (ch))  /* line counter etc. */
/* DISPBASEX reads as base 15:0 / top 31:16 (words) but the observed
 * windows don't bound the live list heads (ch1 head 1636 vs base 2048)
 * — interpretation unresolved, treat as read-only for now. */
#define HVS_DISPBASEX(ch)       (0x004c + 0x10 * (ch))

#define HVS_CHANNELS            3
#define HVS_CHANNEL_HDMI        1       /* firmware routes HDMI = ch1 -> PV2 */

/* Display list RAM: 32-bit words at 0x2000. Heads up to ~2500 words
 * observed live; 4096 words is the conservatively known-readable size. */
#define HVS_DLIST_START         0x2000
#define HVS_DLIST_WORDS         4096

/* ------------------------------------------------------------------ */
/* Display list entry, control word 0                                  */
/* Validated fb plane: 0x47007817 = VALID, 7 words, order 3,           */
/* bits 12:11 = 3 (purpose unconfirmed, kept verbatim), UNITY,          */
/* RGBA8888. Firmware cursor plane uses the identical ctl0.            */
/* ------------------------------------------------------------------ */

#define HVS_CTL0_END            (1UL << 31) /* list terminator word */
#define HVS_CTL0_VALID          (1UL << 30)
#define HVS_CTL0_SIZE_SHIFT     24          /* entry length in words, incl. ctl0 */
#define HVS_CTL0_SIZE_MASK      0x3f
#define HVS_CTL0_ORDER_SHIFT    13          /* pixel channel order; fb uses 3 */
#define HVS_CTL0_ORDER_MASK     0x3
#define HVS_CTL0_UNITY          (1UL << 4)  /* no scaler pipeline (1:1) */
#define HVS_CTL0_FORMAT_MASK    0xf         /* bits 3:0 */

/* The full ctl0 the firmware uses for 32bpp unity planes (both fb and
 * cursor) on Pi3 — reuse verbatim when authoring our own entries. */
#define HVS_CTL0_XRGB_UNITY     0x47007817

/* Firmware's ctl0 for 32bpp scaled (PPF both axes) single-plane
 * entries: size 14, SCL0/SCL1 = 0. Validated in three modes. */
#define HVS_CTL0_XRGB_SCALED    0x4e007807

/* PPF axis word, cracked from firmware entries: bit 30 set, source/dest
 * ratio in 1.16 fixed point at bits 23:8, constant low byte 0x60
 * (phase/AGC). 800/1312=0x9C18 and 1024/1312=0xC7CE both verified. */
#define HVS_PPF(src, dst)       (0x40000000UL | \
                                 ((((ULONG)(src) << 16) / (dst)) & 0xffff) << 8 | \
                                 0x60)

/* Line Buffer Memory allocation for our single scaled plane. Units and
 * pool size are undocumented (firmware used 0x9400-0xa300 for its own
 * scaled fb); we only author a scaled plane on UNITY desktops, so the
 * pool is otherwise unused and base 0 is assumed free. One-line change
 * if real HW disagrees. */
#define HVS_OVL_LBM             0

/* Our copy of the firmware's 11-word Mitchell/Netravali filter kernel.
 * The firmware parks its own at word 0xff4 (4084); ours lives below it,
 * above the list slots (slots end at 4061). Copied at takeover, with
 * HVS_KERNEL_DEFAULT (captured verbatim from a real Pi 3B+) as the
 * fallback when the firmware never initialized one. */
#define HVS_OWN_KERNEL          4064
#define HVS_KERNEL_WORDS        11
#define HVS_FW_KERNEL           0xff4
#define HVS_KERNEL_DEFAULT \
    { 0x07ebfc00, 0x07e3edf8, 0x004805fd, 0x01dca432, 0x0355769b, \
      0x0001c6e3, 0x0355769b, 0x01dca432, 0x004805fd, 0x07e3edf8, \
      0x07ebfc00 }

/* Pixel format codes (bits 3:0 of ctl0) */
#define HVS_PXF_RGB332          0
#define HVS_PXF_RGBA4444        1
#define HVS_PXF_RGB555          2
#define HVS_PXF_RGBA5551        3
#define HVS_PXF_RGB565          4
#define HVS_PXF_RGB888          5
#define HVS_PXF_RGBA6666        6
#define HVS_PXF_RGBA8888        7
#define HVS_PXF_YUV420_3PLANE   8
#define HVS_PXF_YUV420_2PLANE   9
#define HVS_PXF_YUV422_3PLANE   10
#define HVS_PXF_YUV422_2PLANE   11
#define HVS_PXF_H264            12

/* Unity (non-scaled) entry layout, 7 words — all fields validated:
 *   [0] CTL0
 *   [1] POS0   fixed alpha 31:24 | dest y 23:12 | dest x 11:0
 *              (fb sits at 48,48 alpha 0xff = the overscan offset)
 *   [2] POS2   alpha mode 31:30 (fb uses 1) | src height 27:16 | src width 11:0
 *   [3] POS3   context scratch, written by the HVS per frame
 *   [4] PTR0   plane bus address = 0xC0000000 | phys
 *   [5] PTRCTX context scratch, written by the HVS
 *   [6] PITCH0 bytes per row
 *
 * Scaled single-plane RGBA entry, 14 words (validated against the
 * firmware's 1024x768->1312x984 and 1280x720->1749x984 upscale lists):
 *   [0] CTL0 (unity=0, SCL fields set), [1] POS0, [2] POS1 = dest
 *   height 27:16 | dest width 11:0, [3] POS2 (src), [4] POS3 ctx,
 *   [5] PTR0, [6] PTRCTX, [7] PITCH0, then LBM/PPF scaling words.
 * For takeover the entry is inherited verbatim and only PTR0 is ever
 * rewritten, so the scaling words stay opaque. */
#define HVS_UNITY_WORDS         7
#define HVS_SCALED_WORDS        14
#define HVS_FB_ENTRY_MAX        16

#define HVS_POS0_ALPHA_SHIFT    24
#define HVS_POS0_Y_SHIFT        12
#define HVS_POS0_X_SHIFT        0
#define HVS_POS2_AMODE_SHIFT    30
#define HVS_POS2_H_SHIFT        16
#define HVS_POS2_W_SHIFT        0

#define HVS_PTR_BUS_ALIAS       0xC0000000

/* ------------------------------------------------------------------ */
/* PixelValves: bus 0x7E206000 / 0x7E207000 / 0x7E807000               */
/* PV2 (0x7E807000) drives HDMI (confirmed: only enabled PV, carries   */
/* the mode timing); its interrupt is IRQ_PIXELVALVE1 (GPU bank-1      */
/* irq 10) in <hardware/bcm2708.h>, the HVS interrupt is               */
/* IRQ_VIDEOSCALER (bank-1 irq 1).                                     */
/* ------------------------------------------------------------------ */

#define VC4_PV0_BASE            (ARM_PERIIOBASE + 0x206000)
#define VC4_PV1_BASE            (ARM_PERIIOBASE + 0x207000)
#define VC4_PV2_BASE            (ARM_PERIIOBASE + 0x807000)

/* Timing fields validated against 720p50:
 * HORZA=0x00dc0028 (hbp 220 | hsync 40), HORZB=0x01b80500 (hfp 440 |
 * hactive 1280), VERTA=0x00140005 (vbp 20 | vsync 5), VERTB=0x000502d0
 * (vfp 5 | vactive 720). */
#define PV_CONTROL              0x00    /* bit 0 = enable */
#define PV_V_CONTROL            0x04    /* bit 0 = video enable */
#define PV_VSYNCD_EVEN          0x08
#define PV_HORZA                0x0c    /* hsync 15:0, hbp 31:16 (pixels) */
#define PV_HORZB                0x10    /* hactive 15:0, hfp 31:16 (pixels) */
#define PV_VERTA                0x14    /* vsync 15:0, vbp 31:16 (lines) */
#define PV_VERTB                0x18    /* vactive 15:0, vfp 31:16 (lines) */
#define PV_VERTA_EVEN           0x1c
#define PV_VERTB_EVEN           0x20
#define PV_INTEN                0x24
#define PV_INTSTAT              0x28    /* W1C */
#define PV_STAT                 0x2c
#define PV_HACT_ACT             0x30

/* PV_INTEN / PV_INTSTAT bits. Real-Pi3 probe (per-bit arm + IRQ count)
 * confirms the naming: bits 0-3 fire at LINE rate (H events), bit 5
 * (VBP_START, inside vblank) verified at frame rate and used as the
 * vsync tick. INTSTAT does NOT latch events for masked sources (reads
 * 0 with INTEN=0), so hvs_vsync_start() probes by arming one bit at a
 * time and counting deliveries, using the vertical total from
 * PV_VERTA/B as the line/frame ratio. */
#define PV_INT_HSYNC_START      (1 << 0)
#define PV_INT_HBP_START        (1 << 1)
#define PV_INT_HACT_START       (1 << 2)
#define PV_INT_HFP_START        (1 << 3)
#define PV_INT_VSYNC_START      (1 << 4)
#define PV_INT_VBP_START        (1 << 5)
#define PV_INT_VACT_START       (1 << 6)
#define PV_INT_VFP_START        (1 << 7)
#define PV_INT_VFP_END          (1 << 8)
#define PV_INT_IDLE             (1 << 9)

/* ------------------------------------------------------------------ */
/* Display-list ownership (phase 2)                                    */
/* ------------------------------------------------------------------ */

/* Our own display lists live high in the known-readable dlist RAM,
 * clear of the firmware allocator's observed range (~word 2500), in a
 * round-robin of slots so a list being written is never the one the
 * HVS is scanning (it holds at most LACT plus one pending LIST).
 *
 * NOTE: 7 slots, not 8. Slot 7 would start at 3584+7*64=4032 and a full
 * scaled dlist (fb + scaled overlay + cursor + end, ~36 words) reaches
 * 4068 — clobbering HVS_OWN_KERNEL at 4064. That corrupts the PPF filter
 * kernel a scaled overlay scanout reads, wedging the HVS a few frames
 * later (the ~2-3s delay = time to round-robin up to slot 7). Slot 6 ends
 * at 3968+64=4032, clear of the kernel. */
#define HVS_OWN_SLOTS           7
#define HVS_OWN_SLOT_BASE       3584
#define HVS_OWN_SLOT_STRIDE     64

/* Takeover state, embedded in VideoCoreGfx_staticdata. hvs_Active FALSE
 * means the firmware owns the display and all callers must use the
 * mailbox paths. The fb plane words are inherited verbatim from the
 * firmware-built list at takeover. */
struct vc4_hvs_state
{
    BOOL        hvs_Active;
    ULONG       hvs_OutW, hvs_OutH;     /* channel output size */
    ULONG       hvs_FBX, hvs_FBY;       /* fb plane dest position (overscan) */
    ULONG       hvs_DestW, hvs_DestH;   /* fb plane dest size on screen */
    ULONG       hvs_SrcW, hvs_SrcH;     /* fb plane source (fb) size */
    ULONG       hvs_FBEntry[HVS_FB_ENTRY_MAX]; /* inherited fb entry, verbatim */
    ULONG       hvs_FBWords;            /* its length in words */
    ULONG       hvs_FBPtrOff;           /* PTR0 word offset within it */
    ULONG       hvs_FBPtr;              /* ARM phys of current scanout page */
    ULONG       hvs_Slot;               /* next round-robin slot */
    ULONG       hvs_ListBase;           /* head of the current authored list */
    ULONG       hvs_CurOff;             /* cursor entry offset in it, 0 = none */

    /* HVS5 authors its own planes, so their shape is recorded here. */
    ULONG       hvs_CurPtrOff;          /* pointer word within that entry */
    ULONG       hvs_CurWords;           /* its length */
    BOOL        hvs_OvlUsable;          /* fb plane is plain enough to compose over */
    BOOL        hvs_CurShown;           /* last validity written to the cursor plane */

    /* Zero-copy overlay plane (windowed GL): composited above the fb
     * plane, below the cursor. hvs_OvlOff = entry offset in the current
     * list, 0 = none. Dest != src size = HVS-scaled (upscale only). */
    BOOL        hvs_OvlActive;
    ULONG       hvs_OvlPhys, hvs_OvlPitch;
    ULONG       hvs_OvlW, hvs_OvlH;         /* source size */
    ULONG       hvs_OvlDestW, hvs_OvlDestH; /* on-screen size */
    LONG        hvs_OvlX, hvs_OvlY;
    ULONG       hvs_OvlOff;
    ULONG       hvs_OvlWords;               /* entry length in the list */

    /* Filter kernel for scaled planes, copied from the firmware's at
     * takeover; hvs_KernelOK gates scaled-overlay support. */
    BOOL        hvs_KernelOK;

    /* PV2 vsync interrupt: frame counter for flip pacing.
     * hvs_VSyncIrq == NULL means the interrupt is not in use;
     * hvs_VSyncMask is the empirically chosen per-frame INTSTAT bit.
     * hvs_FlipArmed is the counter value at which the most recently
     * armed flip is known to have latched (armed-at + 1). */
    APTR            hvs_VSyncIrq;
    ULONG           hvs_VSyncMask;
    volatile ULONG  hvs_VSyncCount;
    ULONG           hvs_FlipArmed;
};

/* Phase 1: read-only state dump (parses the firmware's live display
 * list, cross-checks against the FBALLOC framebuffer, safe on real HW,
 * self-skips on QEMU). When VC4_HVS_PROBE is enabled it then runs the
 * phase-2 probe: copy the live list, repoint the channel at the copy,
 * verify the HVS latched it, and hand ownership straight back. */
struct VideoCoreGfx_staticdata;
void vc4_hvs_dump(struct VideoCoreGfx_staticdata *xsd,
                  ULONG fb_phys, ULONG fb_pitch,
                  ULONG fb_width, ULONG fb_height);

/* One-time setup at driver init (safe task context): installs the PV2
 * vsync IRQ handler with the interrupt source left masked. Must NOT be
 * deferred into the takeover path — KrnAddIRQHandler allocates memory
 * in supervisor mode and can wedge if called mid mode-set. */
void vc4_hvs_init(struct VideoCoreGfx_staticdata *xsd);

/* Take ownership of the HDMI channel: inherit the fb plane from the
 * firmware list, author our own list (fb + cursor planes), repoint the
 * channel and verify the latch. Leaves the firmware in control (and
 * returns FALSE) on any anomaly. */
BOOL vc4_hvs_takeover(struct VideoCoreGfx_staticdata *xsd,
                      ULONG fb_phys, ULONG fb_pitch);

/* Retarget the fb plane at another framebuffer page (page flip).
 * Returns FALSE when we don't own the display (caller falls back to
 * SETVOFFSET). Caller holds the mailbox lock (vc4_fb_flip). */
BOOL vc4_hvs_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys);

/* Rebuild + repoint after any vcsd_Cur* cursor state change. No-op
 * when not owning. Takes the mailbox lock itself. */
void vc4_hvs_update_cursor(struct VideoCoreGfx_staticdata *xsd);

/* Show (ovl != NULL) or remove (ovl == NULL) the zero-copy overlay
 * plane. Returns FALSE when unavailable (firmware owns the display, or
 * the fb plane is scaled) — caller falls back to blitting. Position
 * updates on a live overlay are patched in place and latch at vblank
 * like page flips. Takes the mailbox lock itself. */
struct vc4gfx_overlay;
BOOL vc4_hvs_overlay(struct VideoCoreGfx_staticdata *xsd,
                     const struct vc4gfx_overlay *ovl);

#endif /* _VIDEOCOREGFX_HVS_H */
