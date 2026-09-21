#ifndef _VIDEOCOREGFX_HVS6_H
#define _VIDEOCOREGFX_HVS6_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 HVS6 register and display-list layout.
*/

#include <exec/types.h>

/* Only +0x0000-0x07ff and +0x4000-0x8dff read back without an abort */
#define HVS6_BASE           (ARM_PERIIOBASE + 0x580000)

#define HVS6_ID             0x00fc
#define HVS6_ID_MAGIC       0x64647276
#define HVS6_VERSION        0x0000      /* bits 7:0: 0x53 = C0, 0x54 = D0 */
#define HVS6_CXM_SIZE       0x0004      /* dlist RAM length, in words     */
#define HVS6_UBM_SIZE       0x000c      /* prefetch buffer arena          */

/* Fixed; CXM_SIZE is the list length, not its base */
#define HVS6_DLIST_WORD     0x1000

/* ch0 drives HDMI0, ch1 HDMI1 */
#define HVS6_CHAN(ch)       (0x0100 + 0x40 * (ch))
#define HVS6_CHANNELS       3
#define HVS6_DISPCTRL       0x0000      /* ENABLE | (w-1)<<16 | (h-1)     */
#define HVS6_DISPLIST       0x0010
#define HVS6_DISPSTAT       0x0018      /* FRCNT / MODE / YLINE           */
#define HVS6_DISPLACT       0x001c      /* DISPLIST as latched            */
#define HVS6_DISPCTRL_EN    (1UL << 31)
/* Live-list writes latch at frame start, when FRCNT advances */
#define HVS6_DISPSTAT_FRCNT(v) (((v) >> 16) & 0x3f)

/* DISPLIST is a word offset from the list base; a repoint never tears */
#define HVS6_DISPCTRL_W(v)  ((((v) >> 16) & 0xfff) + 1)
#define HVS6_DISPCTRL_H(v)  (((v) & 0xffff) + 1)

/* 32-word slots, list ends at an END slot. Firmware lists occupy the
 * first 0x500 words; free RAM keeps the firmware's fill pattern. */
#define HVS6_SLOT_WORDS     32
#define HVS6_FREE_FROM      0x0500
#define HVS6_FILL           0xb0b0b0b0

/* Fixed slot layout so a plane toggles by one CTL0 write; one list per page */
#define HVS6_LIST_SLOTS     4
#define HVS6_SLOT_FB        0
#define HVS6_SLOT_OVL       1
#define HVS6_SLOT_CUR       2
#define HVS6_SLOT_END       3
#define HVS6_LISTS          2

#define HVS6_CTL0_END       (1UL << 31)
#define HVS6_CTL0_VALID     (1UL << 30)
#define HVS6_CTL0_NEXT_SHIFT 24             /* words to the next entry    */
/* Firmware's "size 32, not valid"; the hardware skips it */
#define HVS6_CTL0_EMPTY     ((ULONG)HVS6_SLOT_WORDS << 24)
#define HVS6_CTL0_ALPHAMASK (3UL << 18)     /* 0 = per-pixel, 3 = fixed   */
#define HVS6_CTL0_UNITY     (1UL << 15)     /* source size == dest size   */
#define HVS6_CTL0_RGBA      (3UL << 13)     /* clear = ARGB byte order    */
#define HVS6_CTL0_FMT_MASK  0xf
#define HVS6_CTL0_FMT_8888  7

/* Plane entry. POS0/POS2 pack opposite to HVS5; extents are minus one. */
#define HVS6_ENT_CTL0       0
#define HVS6_ENT_POS0       1           /* y<<16 | x                      */
#define HVS6_ENT_CTL2       2           /* alpha mode, fixed alpha, CSC   */
#define HVS6_ENT_POS2       3           /* (h-1)<<16 | (w-1)              */
#define HVS6_ENT_CTX        4           /* HVS writes the scanline here   */
#define HVS6_ENT_PTR0       5           /* UPM descriptor + addr 39:32    */
#define HVS6_ENT_PTR1       6           /* source address 31:0            */
#define HVS6_ENT_PTR2       7           /* pitch, bytes                   */
#define HVS6_ENT_WORDS      8

#define HVS6_POS0(x, y)     ((((ULONG)(y) & 0xffff) << 16) | ((x) & 0xffff))
#define HVS6_POS2(w, h)     ((((ULONG)(h) - 1) << 16) | (((w) - 1) & 0xffff))

/* PTR0 holds the plane's UPM prefetch slice, not an address. Each live
 * plane needs its own slice and handle. */
#define HVS6_PTR0_BASE(b)   (((ULONG)(b) & 0x1fff) << 16)
#define HVS6_PTR0_HANDLE(h) (((((ULONG)(h)) - 1) & 0x1f) << 10)
#define HVS6_PTR0_LINES2    (0UL << 8)
#define HVS6_PTR0_UPPER(a)  ((ULONG)(((UQUAD)(a) >> 32) & 0xff))

#define HVS6_UPM_GRAN       256             /* allocation unit, bytes     */
#define HVS6_UPM_LINES      2
#define HVS6_UPM_FALLBACK   0x40000         /* arena if UBM_SIZE is odd   */

/* Sized for the widest mode, so a mode change never re-carves */
#define HVS6_UPM_MAX_PITCH  (4096 * 4)
#define HVS6_UPM_SLICE      ((((HVS6_UPM_MAX_PITCH + 62) / 32) * 32 *     \
                              HVS6_UPM_LINES + HVS6_UPM_GRAN - 1) &       \
                             ~(HVS6_UPM_GRAN - 1))

/* Keeps the free-slot search inside the readable range */
#define HVS6_FREE_LIMIT     0x1300

struct VideoCoreGfx_staticdata;

void vc4_hvs6_report(struct VideoCoreGfx_staticdata *xsd, ULONG fb_phys,
                     ULONG fb_pitch, ULONG fb_width, ULONG fb_height);
BOOL vc4_hvs6_takeover(struct VideoCoreGfx_staticdata *xsd, ULONG fb_phys,
                       ULONG fb_pitch, ULONG fb_width, ULONG fb_height);
BOOL vc4_hvs6_add_backpage(struct VideoCoreGfx_staticdata *xsd,
                           ULONG fb_pitch, ULONG fb_height);
BOOL vc4_hvs6_flip_page(struct VideoCoreGfx_staticdata *xsd, ULONG page_phys);
void vc4_hvs6_latch_wait(struct VideoCoreGfx_staticdata *xsd);
BOOL vc4_hvs6_overlay(struct VideoCoreGfx_staticdata *xsd,
                      struct vc4gfx_overlay *ovl);
BOOL vc4_hvs6_init_cursor(struct VideoCoreGfx_staticdata *xsd);
void vc4_hvs6_cursor(struct VideoCoreGfx_staticdata *xsd);
void vc4_hvs6_release(struct VideoCoreGfx_staticdata *xsd);

#endif /* _VIDEOCOREGFX_HVS6_H */
