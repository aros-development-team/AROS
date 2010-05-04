/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"

#include "i915_drv.h"
#include "i915_pciids.h"

/* FIXME: What this does? */
unsigned int i915_powersave = 0;

//extern struct drm_ioctl_desc 915_ioctls[];

static struct drm_driver driver =
{
    .VendorID = 0x8086,
    .ProductID = 0x0,
    .PciIDs = i915_pciids,
    .pciDevice = NULL,
    .IsAGP = FALSE,
    .IsPCIE = FALSE,
    .dev = NULL,
    .driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_USE_AGP | DRIVER_REQUIRE_AGP,
    .load = i915_driver_load,
    .firstopen = NULL,
    .open = i915_driver_open,
    .preclose = i915_driver_preclose,
    .postclose = i915_driver_postclose,
    .lastclose = i915_driver_lastclose,
    .unload = i915_driver_unload,
    .irq_handler = i915_driver_irq_handler,
    .irq_preinstall = i915_driver_irq_preinstall,
    .irq_postinstall = i915_driver_irq_postinstall,
    .irq_uninstall = i915_driver_irq_uninstall,
/*    .version_patchlevel = NOUVEAU_DRM_HEADER_PATCHLEVEL,
    .ioctls = nouveau_ioctls,*/
    .gem_init_object =  i915_gem_init_object,
    .gem_free_object = i915_gem_free_object,
};

void i915_exit(void)
{
    drm_exit(&driver);
}

int i915_init(void)
{
    return drm_init(&driver);
}
