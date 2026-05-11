/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_PCI_
#define _DRM_COMPAT_PCI_

#include <exec/types.h>

#include <drm-compat/drm_compat_types.h>

/* PCI support */
struct pci_dev
{
    ULONG class;
    APTR oopdev;
    TEXT name[16];
};

#define pci_map_page(a, b, c, d, e)     (dma_addr_t)(b->address + c)
#define pci_dma_mapping_error(a, b)     FALSE
#define pci_unmap_page(a, b, c, d)      
resource_size_t pci_resource_start(struct pci_dev * pdev, unsigned int barnum);
unsigned long pci_resource_len(struct pci_dev * pdev, unsigned int barnum);
#define PCI_DEVFN(dev, fun)             dev, fun
void * pci_get_bus_and_slot(unsigned int bus, unsigned int dev, unsigned int fun);
int pci_read_config_word(struct pci_dev * pdev, int where, u16 *val);
int pci_read_config_dword(struct pci_dev * pdev, int where, u32 *val);
int pci_write_config_dword(struct pci_dev * pdev, int where, u32 val);
#define pci_name(pdev)                  ((const char *)pdev->name)
int pci_is_pcie(struct pci_dev * pdev);

#endif /* _DRM_COMPAT_PCI_ */
