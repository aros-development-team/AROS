/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"

#include "nouveau_drv.h"
#include "nouveau_pciids.h"

/* Whether pushbuf and notifer are to be put in VRAM (access via 
   pci mapping) or in GART (accessed by card - SGDMA or AGP)*/
/* Setting nouveau_vram_pushbuf to 1 causes problems */
int nouveau_vram_pushbuf = 0;
int nouveau_vram_notify = 0;
int nouveau_noaccel = 0;
char *nouveau_vbios = NULL; /* Override default VBIOS location */
/* Register access debug bitmask:
0x1 mc, 0x2 video, 0x4 fb, 0x8 extdev, 0x10 crtc, 0x20 ramdac, 0x40 vgacrtc, 
0x80 rmvio, 0x100 vgaattr, 0x200 EVO (G80+) */
int nouveau_reg_debug = 0;
int nouveau_duallink = 1; /* Enable dual-link >= GeForce 8 */
/* "Default TV norm.
Supported: PAL, PAL-M, PAL-N, PAL-Nc, NTSC-M, NTSC-J, hd480i, hd480p, hd576i, 
hd576p, hd720p, hd1080i
Default: PAL"
*NOTE* Ignored for cards with external TV encoders.") */
char * nouveau_tv_norm = NULL;
/* LVDS output script table ID (>=GeForce 8) */
int nouveau_uscript_lvds = -1;
/* TMDS output script table ID (>=GeForce 8) */
int nouveau_uscript_tmds = -1;
/* Ignore DCB connector type */
int nouveau_override_conntype = 0;
/* Disable TV-out detection */
int nouveau_tv_disable = 0;
/* 0 to disable AGP */
int nouveau_agpmode = -1;
/* Enable MSI */
int nouveau_msi = 0;
/* Force POST */
int nouveau_force_post = 0;
/* DRM Debug, Bits: 0x01 | 0x02 | 0x04 */
unsigned int drm_debug = 0;
/* Use external firmware */
int nouveau_ctxfw = 0;


extern struct drm_ioctl_desc nouveau_ioctls[];

static struct drm_driver driver =
{
    .VendorID = 0x10de,
    .ProductID = 0x0,
    .PciIDs = nouveau_pciids,
    .pciDevice = NULL,
    .IsAGP = FALSE,
    .IsPCIE = FALSE,
    .dev = NULL,
    .driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_USE_AGP,
    .load = nouveau_load,
    .firstopen = nouveau_firstopen,
    .open = nouveau_open,
    .preclose = nouveau_preclose,
    .postclose = NULL,
    .lastclose = nouveau_lastclose,
    .unload = nouveau_unload,
    .irq_handler = nouveau_irq_handler,
    .irq_preinstall = nouveau_irq_preinstall,
    .irq_postinstall = nouveau_irq_postinstall,
    .irq_uninstall = nouveau_irq_uninstall,
    .version_patchlevel = NOUVEAU_DRM_HEADER_PATCHLEVEL,
    .ioctls = nouveau_ioctls,
    .gem_init_object = nouveau_gem_object_new,
    .gem_free_object = nouveau_gem_object_del,
    .gem_open_object = nouveau_gem_object_open,
    .gem_close_object = nouveau_gem_object_close,
};

void nouveau_exit(void)
{
    drm_exit(&driver);
}

int nouveau_init(void)
{
    return drm_init(&driver);
}
