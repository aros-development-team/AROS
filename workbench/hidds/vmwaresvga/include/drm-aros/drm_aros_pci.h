/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#if !defined(DRM_AROS_H)
#define DRM_AROS_H

#include <oop/oop.h>

extern OOP_Object *pciDriver;
extern OOP_Object *pciBus;
struct pci_dev;

struct pci_dev *drm_aros_pci_find_supported_video_card();
VOID drm_aros_pci_shutdown();
LONG drm_aros_pci_init();

#endif /* DRM_AROS_H */
