/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_APERTURE_H_
#define _LINUX_APERTURE_H_

#include <linux/pci.h>
#include <linux/types.h>
static inline int aperture_remove_conflicting_devices(resource_size_t base, resource_size_t size, const char *name) { return 0; }
static inline int aperture_remove_conflicting_pci_devices(struct pci_dev *pdev, const char *name) { return 0; }
static inline int __aperture_remove_legacy_vga_devices(struct pci_dev *pdev) { return 0; }
static inline int devm_aperture_acquire_for_platform_device(void *pdev, resource_size_t base, resource_size_t size) { return 0; }

#endif /* _LINUX_APERTURE_H_ */
