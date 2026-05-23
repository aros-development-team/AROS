/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_PCI_
#define _DRM_COMPAT_PCI_

#include <exec/types.h>

#include <drm-compat/drm_compat_types.h>

/* PCI support */
struct pci_device_id
{
    IPTR driver_data;
};

struct pci_dev
{
    struct device dev;

    ULONG class;
    APTR oopdev;
    TEXT name[16];
    BOOL isAGP;
    BOOL isPCIE;

    UWORD   vendor;
    UWORD   device;
    UWORD   subsystem_vendor;
    UWORD   subsystem_device;
    UWORD   irq;
    IPTR    rom;
    size_t  romlen;
};

#define PCI_SLOT(devfn)     (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)     ((devfn) & 0x07)

#define pci_map_page(a, b, c, d, e)     dma_map_page(&a->dev, b, c, d, e)
#define pci_dma_mapping_error(a, b)     FALSE
#define pci_unmap_page(a, b, c, d)      
resource_size_t pci_resource_start(struct pci_dev * pdev, unsigned int barnum);
unsigned long pci_resource_len(struct pci_dev * pdev, unsigned int barnum);
void * pci_get_bus_and_slot(unsigned int bus, unsigned int dev, unsigned int fun);
int pci_read_config_word(struct pci_dev * pdev, int where, u16 *val);
int pci_read_config_dword(struct pci_dev * pdev, int where, u32 *val);
int pci_write_config_dword(struct pci_dev * pdev, int where, u32 val);
#define pci_name(pdev)                  ((const char *)pdev->name)
int pci_is_pcie(struct pci_dev * pdev);
void pci_disable_device(struct pci_dev *pdev);
int pci_enable_device(struct pci_dev *pdev);
int pci_set_master(struct pci_dev *pdev);
int pci_enable_msi(struct pci_dev *pdev);
int pci_enable_rom(struct pci_dev *pdev);
void pci_disable_rom(struct pci_dev *pdev);
void *pci_map_rom(struct pci_dev *pdev, size_t *size);
void pci_unmap_rom(struct pci_dev *pdev, void *mem);

#endif /* _DRM_COMPAT_PCI_ */
