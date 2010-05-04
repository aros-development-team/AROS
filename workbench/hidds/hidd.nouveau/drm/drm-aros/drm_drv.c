/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"
#include "drm_aros.h"

struct drm_driver *current_drm_driver = NULL;

int drm_lastclose(struct drm_device * dev)
{
    if (dev->driver->lastclose)
        dev->driver->lastclose(dev);
    
    if (dev->irq_enabled)
        drm_irq_uninstall(dev);
    
    return 0;
}

static void drm_cleanup(struct drm_device * dev)
{
    drm_lastclose(dev);
    
    if (dev->driver->unload)
        dev->driver->unload(dev);
        
    if (drm_core_has_AGP(dev) && dev->agp)
    {
        kfree(dev->agp);
        dev->agp = NULL;
    }
}

void drm_exit(struct drm_driver * driver)
{
    drm_cleanup(driver->dev);
    
    drm_aros_pci_shutdown(driver);
    
    FreeVec(driver->dev);
    driver->dev = NULL;
    current_drm_driver = NULL;
}

static int drm_init_device(struct drm_driver * driver)
{
    /* If this function is called, the card was already found */
    driver->dev = AllocVec(sizeof(struct drm_device), MEMF_ANY | MEMF_CLEAR);
    struct drm_device * dev = driver->dev;
    
    /* Init fields */
    INIT_LIST_HEAD(&dev->maplist);
    dev->irq_enabled = 0;
    InitSemaphore(&dev->struct_mutex.semaphore);
    dev->driver = driver;
    dev->pci_vendor = driver->VendorID;
    dev->pci_device = driver->ProductID;
    dev->pdev = driver->pciDevice;
    int ret;

    if (drm_core_has_AGP(dev)) {
        if (drm_device_is_agp(dev))
            dev->agp = drm_agp_init(dev);
        if (drm_core_check_feature(dev, DRIVER_REQUIRE_AGP)
            && (dev->agp == NULL)) {
            DRM_ERROR("Cannot initialize the agpgart module.\n");
            return -1;
        }
    }
    
    if (driver->driver_features & DRIVER_GEM) {
        if (drm_gem_init(dev)) {
            DRM_ERROR("Cannot initialize graphics execution "
                  "manager (GEM)\n");
            return -1;
        }
    }
    
    if (!dev->driver->load)
        return -1;

    ret = dev->driver->load(dev, 0);
    if (ret)
        return -1;

    if (dev->driver->firstopen)
    {    
        ret = dev->driver->firstopen(dev);
        if (ret)
            return -1;
    }
    
    return 0;
}

int drm_init(struct drm_driver * driver)
{
    if (drm_aros_pci_init(driver))
        return -1;

#if !defined(HOSTED_BUILD)
    if (drm_aros_pci_find_supported_video_card(driver))
        return -1;
#else
#if HOSTED_BUILD_BUS == HOSTED_BUILD_BUS_PCI
    driver->IsAGP = FALSE;
#endif
#if HOSTED_BUILD_BUS == HOSTED_BUILD_BUS_AGP
    driver->IsAGP = TRUE;
#endif
#if HOSTED_BUILD_HARDWARE == HOSTED_BUILD_HARDWARE_I915
    if (driver->VendorID != 0x8086) return -1;
#endif
#if HOSTED_BUILD_HARDWARE == HOSTED_BUILD_HARDWARE_NVIDIA
    if (driver->VendorID != 0x10de) return -1;
#endif
#if HOSTED_BUILD_HARDWARE == HOSTED_BUILD_HARDWARE_I915
    driver->ProductID = HOSTED_BUILD_PRODUCT_ID;
    driver->IsAGP = TRUE; /* AGP is needed for INTEL */
#endif
#endif
    if (drm_init_device(driver))
    {
        drm_exit(driver);
        return -1;
    }
    
    current_drm_driver = driver;
    
    return 0;
}
