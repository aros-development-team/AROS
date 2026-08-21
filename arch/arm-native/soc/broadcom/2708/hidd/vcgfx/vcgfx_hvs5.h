#ifndef _VIDEOCOREGFX_HVS5_H
#define _VIDEOCOREGFX_HVS5_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2711 (VideoCore VI) HVS5 and PixelValve addresses.

    Every address and interrupt below is read straight out of the Pi 4
    firmware device tree (bcm2711-rpi-4-b.dtb), so they are plain ABI
    facts. The register layout inside the block is NOT assumed anywhere -
    vc4_hvs5_dump() finds the display list by searching for the known
    framebuffer address, which is what phase 2 will be built on.
*/

#include <exec/types.h>

/* hvs@7e400000, "brcm,bcm2711-hvs": same base as VideoCore IV, but a
 * 0x8000 register window instead of 0x6000. */
#define HVS5_BASE           (ARM_PERIIOBASE + 0x400000)
#define HVS5_REG_SIZE       0x8000
#define HVS5_IRQ            (32 + 97)       /* GIC SPI 97 */

/* Control registers live low in the window; the display list RAM sits
 * somewhere above. Scan a generous span: VideoCore IV kept its lists at
 * 0x2000, HVS5 is documented higher, and the search settles it. */
#define HVS5_SCAN_START     0x2000
#define HVS5_SCAN_END       HVS5_REG_SIZE

/* ------------------------------------------------------------------ */
/* Decoded from a live firmware list on a real Pi 4 (2026-08-20,       */
/* 1280x1024 fb + 32x32 cursor on HDMI). Everything below is a         */
/* measurement, not an assumption.                                     */
/* ------------------------------------------------------------------ */

/* The global block kept the VideoCore IV layout: DISPCTRL at +0x00,
 * DISPSTAT at +0x04, and the same 0x64647276 identity at +0x08. The
 * per-channel list head registers sit where they always did. */
#define HVS5_ID             0x0008
#define HVS5_ID_MAGIC       0x64647276
#define HVS5_DISPLIST(ch)   (0x0020 + 4 * (ch))
#define HVS5_DISPLACT(ch)   (0x0030 + 4 * (ch))
#define HVS5_DISPCTRLX(ch)  (0x0040 + 0x10 * (ch))
#define HVS5_CHANNELS       3

/* Firmware drove HDMI on channel 0, with both LIST and LACT reading 4. */
#define HVS5_CHANNEL_HDMI   0

/* The one structural move: list RAM is at +0x4000, not +0x2000, and
 * heads are word indices into it (head 4 -> +0x4010). */
#define HVS5_DLIST_START    0x4000
#define HVS5_DLIST_WORDS    4096

/* A 32bpp plane entry is 8 words, one more than VideoCore IV's 7, with
 * an extra field at [2] (0x4000fff0 for the framebuffer plane against
 * 0x0000fff0 for the cursor - bit 30 carries the alpha mode the VC4
 * entry kept in POS2). The observed framebuffer entry:
 *
 *   [0] CTL0   0x4800d807  VALID | size 8 | RGBA8888
 *   [1] POS0   0x00000000  dest y 31:16 | x 15:0
 *   [2]        0x4000fff0  alpha mode 31:30
 *   [3] POS2   0x04000500  height 31:16 | width 11:0  (1280x1024)
 *   [4]        0x014f0000  context, HVS-written
 *   [5] PTR0   0xfdcf5000  plane address, 0xC0000000 alias as on VC4
 *   [6] PTRCTX 0xfdf9f800  context, HVS-written
 *   [7] PITCH  0x00001400  5120 bytes
 *
 * The tail is anchored to the end of the entry exactly as on VideoCore
 * IV, so one rule covers both: PTR0 at [size-3], PTRCTX at [size-2],
 * PITCH at [size-1]. That is what a takeover needs - inherit the entry
 * verbatim and rewrite PTR0 alone.
 *
 * POS0 packs into 16-bit halves like POS2, NOT into VideoCore IV's
 * 12-bit fields: a cursor plane read back as 0x02000283 with the pointer
 * near the middle of a 1280x1024 screen decodes to x 643, y 512, where
 * the VC4 packing would have claimed y 0. Widening these fields is
 * presumably why the entry grew a word. */
#define HVS5_POS0(x, y)     ((((ULONG)(y) & 0xffff) << 16) \
                             | ((ULONG)(x) & 0xffff))
#define HVS5_POS2(w, h)     ((((ULONG)(h) & 0xffff) << 16) \
                             | ((ULONG)(w) & 0xffff))

/* Template for a cursor plane of our own, taken field for field from the
 * firmware's. Word [2] selects how the plane blends: the framebuffer used
 * 0x4000fff0 and the cursor 0x0000fff0, so bit 30 is fixed alpha against
 * per-pixel. [4] and [6] are context the HVS writes itself and start at
 * zero, as they do on VideoCore IV. */
#define HVS5_CURSOR_WORDS   8
#define HVS5_CTL0_CURSOR    0x4800d807
#define HVS5_ALPHA_PERPIXEL 0x0000fff0
#define HVS5_ALPHA_FIXED    0x4000fff0
#define HVS5_CTL0_END           (1UL << 31)
#define HVS5_CTL0_VALID         (1UL << 30)
#define HVS5_CTL0_SIZE_SHIFT    24
#define HVS5_CTL0_SIZE_MASK     0x3f
#define HVS5_CTL0_FORMAT_MASK   0xf
#define HVS5_PXF_RGBA8888       7
#define HVS5_PTR_BUS_ALIAS      0xC0000000
#define HVS5_PTROFF_FROM_END    3       /* PTR0 = entry[size - 3] */

/* Filter kernel: scaled entries pointed at word 0xff4, same as VC4, so
 * the free run below it can be carved up the same way. */
#define HVS5_FW_KERNEL          0xff4

/* pixelvalve@..., "brcm,bcm2711-pixelvalveN". PV2 has moved (0x7e807000
 * on BCM283x) and PV3/PV4 are new.
 *
 * Measured on hardware: PV4 (0x7e216000) is the enabled one, and it
 * counts two pixels per clock horizontally - a 1280x1024 screen reports
 * hactive 640, and doubling every horizontal field reproduces the VESA
 * timing exactly (hsync 112, hbp 248, hfp 48, htotal 1688). Vertical
 * fields are not scaled. Note PV1 and PV4 share GIC SPI 110. */
#define HVS5_PV_HDMI        4
#define HVS5_PV_HDMI_IRQ    (32 + 110)
#define HVS5_PV_H_PIXELS_PER_CLK 2
struct hvs5_pv
{
    const char *pv_Name;
    ULONG       pv_Offset;
    ULONG       pv_Irq;
};

#define HVS5_PV_COUNT       5

/* Read-only bring-up dump: HVS control registers, then the display list
 * located by searching for fb_phys, then every PixelValve. Safe on real
 * hardware, and the only way to learn the HVS5 layout - QEMU's raspi4b
 * has no HVS model. */
struct VideoCoreGfx_staticdata;
void vc4_hvs5_dump(struct VideoCoreGfx_staticdata *xsd,
                   ULONG fb_phys, ULONG fb_pitch,
                   ULONG fb_width, ULONG fb_height);

/* Our own lists live high in list RAM, clear of the firmware's (which
 * ran from word 4 to about word 840) and of the filter kernel at 0xff4.
 * Two slots so a mode switch never rewrites the list being scanned. */
#define HVS5_OWN_SLOTS      2
#define HVS5_OWN_SLOT_BASE  3584
#define HVS5_OWN_SLOT_STRIDE 128

/* Take the HDMI channel: copy the firmware's live list into a slot of
 * our own, retarget the framebuffer plane and repoint the channel. The
 * whole list is inherited verbatim - the cursor plane included - so
 * nothing depends on knowing how to author an HVS5 entry. Returns FALSE
 * and leaves the firmware in charge on any anomaly. */
BOOL vc4_hvs5_takeover(struct VideoCoreGfx_staticdata *xsd,
                       ULONG fb_phys, ULONG fb_pitch);

/* Retarget the framebuffer plane at another page. One word, latched by
 * the HVS at frame start. */
BOOL vc4_hvs5_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys);

/* Patch the inherited cursor plane in place. No-op when the firmware had
 * no cursor plane at takeover. */
void vc4_hvs5_update_cursor(struct VideoCoreGfx_staticdata *xsd);

#endif /* _VIDEOCOREGFX_HVS5_H */
