#ifndef _DRM_MODULE_H_
#define _DRM_MODULE_H_
/* No module loader: the hidd calls the probe itself. */
#define drm_module_pci_driver(drv)
#define drm_module_pci_driver_if_modeset(drv, m)
#define drm_module_platform_driver(drv)
#endif
