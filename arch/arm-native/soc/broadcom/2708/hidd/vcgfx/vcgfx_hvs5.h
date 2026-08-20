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

/* pixelvalve@..., "brcm,bcm2711-pixelvalveN". PV2 has moved (0x7e807000
 * on BCM283x) and PV3/PV4 are new. Which one feeds HDMI0 is what the
 * dump answers - only an enabled PV carries mode timing. */
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

#endif /* _VIDEOCOREGFX_HVS5_H */
