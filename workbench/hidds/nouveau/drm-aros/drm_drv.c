/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <drm-compat/drm_compat_funcs.h>
#include <drm-compat/drm_compat_pci.h>
#include <drm-compat/drm_compat_mem.h>
#include <drm-aros/drm_aros_pci.h>
#include <uapi/drm/drm.h>
#include <drm/drm_drv.h>
#include <drm/drm_print.h>
#include <linux/err.h>

int drm_modeset_register_all(struct drm_device *dev);
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
    mutex_init(&dev->struct_mutex);
    
    if (drm_core_check_feature(dev, DRIVER_GEM)) {
        if (drm_gem_init(dev)) {
            DRM_ERROR("Cannot initialize graphics execution "
                  "manager (GEM)\n");
            return -1;
        }
    }

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

int drm_dev_register(struct drm_device *dev, unsigned long flags)
{
    int ret = 0;

    dev->registered = true;
    if (drm_core_check_feature(dev, DRIVER_MODESET))
    {
        ret = drm_modeset_register_all(dev);
        if (ret)
            goto err_unload;
    }

    return ret;
err_unload:
    return ret;
}

void drm_dev_get(struct drm_device *dev)
{
    /* No Op */
}

void drm_dev_put(struct drm_device *dev)
{
    /* No Op */
}

int nouveau_drm_probe(struct pci_dev *pdev, const struct pci_device_id *pent, struct drm_device **pdrm_dev);
struct drm_device *current_drm_device;
BOOL workqueue_init();

/*
 * Bring-up is split in two so that the caller gets a look at the card -
 * and in particular at its BARs - after it has been found but before it
 * has been touched. Whoever the firmware left driving this hardware has
 * to be shut down in that gap: nouveau_init_probe() reprograms the card,
 * and any boot-mode driver still writing through the old apertures
 * afterwards writes into the new owner's state.
 */
struct pci_dev *nouveau_init_findcard(void)
{
    if (drm_aros_pci_init())
        return NULL;

    return drm_aros_pci_find_supported_video_card();
}

int nouveau_init_probe(struct pci_dev *pdev)
{
    struct pci_device_id dummy;

    if (!pdev)
        return -1;

    if (!workqueue_init())
        return -1;

    bug("\003\n"); /* Tell vga text mode debug output to die */

    if (nouveau_drm_probe(pdev, &dummy, &current_drm_device))
        return -1;

    return 0;
}

int nouveau_init()
{
    return nouveau_init_probe(nouveau_init_findcard());
}
