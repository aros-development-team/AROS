/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <drm-compat/drm_compat_funcs.h>
#include <drm-compat/drm_compat_mem.h>
#include <uapi/drm/drm.h>
#include <drm/drm_drv.h>
#include <drm/drm_print.h>

// int drm_lastclose(struct drm_device * dev)
// {
//     if (dev->driver->lastclose)
//         dev->driver->lastclose(dev);
    
//     if (dev->irq_enabled)
//         drm_irq_uninstall(dev);
    
//     return 0;
// }

// static void drm_cleanup(struct drm_device * dev)
// {
//     drm_lastclose(dev);
    
//     if (dev->driver->unload)
//         dev->driver->unload(dev);
        
//     if (drm_core_has_AGP(dev) && dev->agp)
//     {
//         kfree(dev->agp);
//         dev->agp = NULL;
//     }
// }

// void drm_exit(struct drm_driver * driver)
// {
//     drm_cleanup(driver->dev);
    
//     drm_aros_pci_shutdown(driver);
    
//     HIDDNouveauFree(driver->dev->pdev);
//     HIDDNouveauFree(driver->dev);
//     driver->dev = NULL;
//     current_drm_driver = NULL;
// }

int drm_gem_init(struct drm_device *dev);

static int drm_dev_init(struct drm_device *dev, struct drm_driver *driver,
    struct device *parent)
{
    dev->dev = NULL;
    dev->driver = driver;
    dev->dev_private = NULL;
    dev->irq_enabled = 0;


    dev->driver_features = ~0u;

    // /* Init fields */
    // INIT_LIST_HEAD(&dev->maplist);
    // InitSemaphore(&dev->struct_mutex.semaphore);
    // int ret;

    // if (drm_core_has_AGP(dev)) {
    //     if (drm_device_is_agp(dev))
    //         dev->agp = drm_agp_init(dev);
    //     if (drm_core_check_feature(dev, DRIVER_REQUIRE_AGP)
    //         && (dev->agp == NULL)) {
    //         DRM_ERROR("Cannot initialize the agpgart module.\n");
    //         return -1;
    //     }
    // }
    
    if (drm_core_check_feature(dev, DRIVER_GEM)) {
        if (drm_gem_init(dev)) {
            DRM_ERROR("Cannot initialize graphics execution "
                  "manager (GEM)\n");
            return -1;
        }
    }
    
    // if (!dev->driver->load)
    //     return -1;

    // ret = dev->driver->load(dev, 0);
    // if (ret)
    //     return -1;

    // if (dev->driver->firstopen)
    // {
    //     ret = dev->driver->firstopen(dev);
    //     if (ret)
    //         return -1;
    // }
    
    return 0;
}

struct drm_device *drm_dev_alloc(struct drm_driver *driver,
				 struct device *parent)
{
	struct drm_device *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return ERR_PTR(-ENOMEM);

	ret = drm_dev_init(dev, driver, parent);
	if (ret) {
		kfree(dev);
		return ERR_PTR(ret);
	}

	return dev;
}

// int drm_aros_init(struct drm_driver * driver)
// {
//     if (drm_aros_pci_init(driver))
//         return -1;

//     if (drm_aros_pci_find_supported_video_card(driver))
//         return -1;

//     bug("\003\n"); /* Tell vga text mode debug output to die */

//     if (drm_init_device(driver))
//     {
//         drm_exit(driver);
//         return -1;
//     }
    
//     current_drm_driver = driver;
    
//     return 0;
// }
