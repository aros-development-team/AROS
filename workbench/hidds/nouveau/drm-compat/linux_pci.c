/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/pci.h>
#include <hidd/hidd.h>

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include <drm-aros/drm_aros_pci.h>

/*
 * Everything PCI goes through the pci.hidd objects that drm_aros_pci.c
 * discovered: pciDriver is the bus driver, pdev->oopdev the device.
 */

const char *dev_name(const struct device *dev)
{
    if (!dev)
        return "nouveau";
    if (dev->is_pci) {
        const struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
        return pdev->name;
    }
    if (dev->init_name)
        return dev->init_name;
    return "nouveau";
}

void __iomem *ioremap(resource_size_t offset, unsigned long size)
{
    if (!pciDriver) {
        bug("[nouveau] ioremap used without a PCI driver\n");
        return NULL;
    }
    return HIDD_PCIDriver_MapPCI(pciDriver, (APTR)(IPTR)offset, size);
}

void iounmap(volatile void __iomem *addr)
{
    if (pciDriver && addr)
        HIDD_PCIDriver_UnmapPCI(pciDriver, (APTR)addr, 0);
}

void *pci_resource_cpu_addr(resource_size_t busaddr)
{
    if (pciDriver)
        return HIDD_PCIDriver_PCItoCPU(pciDriver, (APTR)(IPTR)busaddr);
    return (void *)(IPTR)busaddr;
}

void memcpy_fromio(void *dst, const volatile void __iomem *src, size_t count)
{
    const volatile u8 *s = src;
    u8 *d = dst;

    while (count && ((IPTR)s & 3)) { *d++ = readb(s++); count--; }
    while (count >= 4) { *(u32 *)d = __raw_readl(s); d += 4; s += 4; count -= 4; }
    while (count--) *d++ = readb(s++);
    __io_ar();
}

void memcpy_toio(volatile void __iomem *dst, const void *src, size_t count)
{
    volatile u8 *d = dst;
    const u8 *s = src;

    __io_bw();
    while (count && ((IPTR)d & 3)) { __raw_writeb(*s++, d++); count--; }
    while (count >= 4) { __raw_writel(*(const u32 *)s, d); d += 4; s += 4; count -= 4; }
    while (count--) __raw_writeb(*s++, d++);
}

void memset_io(volatile void __iomem *dst, int c, size_t count)
{
    volatile u8 *d = dst;
    u32 v32 = (u8)c * 0x01010101u;

    __io_bw();
    while (count && ((IPTR)d & 3)) { __raw_writeb(c, d++); count--; }
    while (count >= 4) { __raw_writel(v32, d); d += 4; count -= 4; }
    while (count--) __raw_writeb(c, d++);
}

resource_size_t pci_resource_start(struct pci_dev *pdev, unsigned int bar)
{
    IPTR start = 0;
    static const ULONG attrs[] = { 0, 1, 2, 3, 4, 5 };
    ULONG attr;

    if (!pdev || !pdev->oopdev)
        return 0;
    switch (bar) {
    case 0: attr = aHidd_PCIDevice_Base0; break;
    case 1: attr = aHidd_PCIDevice_Base1; break;
    case 2: attr = aHidd_PCIDevice_Base2; break;
    case 3: attr = aHidd_PCIDevice_Base3; break;
    case 4: attr = aHidd_PCIDevice_Base4; break;
    case 5: attr = aHidd_PCIDevice_Base5; break;
    case PCI_ROM_RESOURCE: attr = aHidd_PCIDevice_RomBase; break;
    default: return 0;
    }
    (void)attrs;
    OOP_GetAttr((OOP_Object *)pdev->oopdev, attr, &start);
    return start;
}

resource_size_t pci_resource_len(struct pci_dev *pdev, unsigned int bar)
{
    IPTR len = 0;
    ULONG attr;

    if (!pdev || !pdev->oopdev)
        return 0;
    if (pci_resource_start(pdev, bar) == 0)
        return 0;
    switch (bar) {
    case 0: attr = aHidd_PCIDevice_Size0; break;
    case 1: attr = aHidd_PCIDevice_Size1; break;
    case 2: attr = aHidd_PCIDevice_Size2; break;
    case 3: attr = aHidd_PCIDevice_Size3; break;
    case 4: attr = aHidd_PCIDevice_Size4; break;
    case 5: attr = aHidd_PCIDevice_Size5; break;
    case PCI_ROM_RESOURCE: attr = aHidd_PCIDevice_RomSize; break;
    default: return 0;
    }
    OOP_GetAttr((OOP_Object *)pdev->oopdev, attr, &len);
    return len;
}

unsigned long pci_resource_flags(struct pci_dev *pdev, unsigned int bar)
{
    IPTR type = 0;
    ULONG attr;

    if (!pdev || !pdev->oopdev || pci_resource_start(pdev, bar) == 0)
        return 0;
    switch (bar) {
    case 0: attr = aHidd_PCIDevice_Type0; break;
    case 1: attr = aHidd_PCIDevice_Type1; break;
    case 2: attr = aHidd_PCIDevice_Type2; break;
    case 3: attr = aHidd_PCIDevice_Type3; break;
    case 4: attr = aHidd_PCIDevice_Type4; break;
    case 5: attr = aHidd_PCIDevice_Type5; break;
    default: return IORESOURCE_MEM;
    }
    OOP_GetAttr((OOP_Object *)pdev->oopdev, attr, &type);
    if (type & ADDRF_IO)
        return IORESOURCE_IO;
    /* the type attribute is the raw low BAR bits; 2:1 == 2 marks a 64-bit BAR */
    return IORESOURCE_MEM | ((type & ADDRF_PREFETCH) ? IORESOURCE_PREFETCH : 0)
                          | (((type & 0x6) == 0x4) ? IORESOURCE_MEM_64 : 0);
}

int pci_read_config_byte(struct pci_dev *pdev, int where, u8 *val)
{
    *val = HIDD_PCIDevice_ReadConfigByte((OOP_Object *)pdev->oopdev, where);
    return 0;
}

int pci_read_config_word(struct pci_dev *pdev, int where, u16 *val)
{
    *val = HIDD_PCIDevice_ReadConfigWord((OOP_Object *)pdev->oopdev, where);
    return 0;
}

int pci_read_config_dword(struct pci_dev *pdev, int where, u32 *val)
{
    *val = HIDD_PCIDevice_ReadConfigLong((OOP_Object *)pdev->oopdev, where);
    return 0;
}

int pci_write_config_byte(struct pci_dev *pdev, int where, u8 val)
{
    HIDD_PCIDevice_WriteConfigByte((OOP_Object *)pdev->oopdev, where, val);
    return 0;
}

int pci_write_config_word(struct pci_dev *pdev, int where, u16 val)
{
    HIDD_PCIDevice_WriteConfigWord((OOP_Object *)pdev->oopdev, where, val);
    return 0;
}

int pci_write_config_dword(struct pci_dev *pdev, int where, u32 val)
{
    HIDD_PCIDevice_WriteConfigLong((OOP_Object *)pdev->oopdev, where, val);
    return 0;
}

/* raw bus/devfn access, for the bridge/upstream lookups */
int pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 *val)
{
    if (!pciDriver) return -ENODEV;
    *val = HIDD_PCIDriver_ReadConfigByte(pciDriver, NULL, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), where);
    return 0;
}
int pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 *val)
{
    if (!pciDriver) return -ENODEV;
    *val = HIDD_PCIDriver_ReadConfigWord(pciDriver, NULL, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), where);
    return 0;
}
int pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 *val)
{
    if (!pciDriver) return -ENODEV;
    *val = HIDD_PCIDriver_ReadConfigLong(pciDriver, NULL, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), where);
    return 0;
}
int pci_bus_write_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 val)
{
    if (!pciDriver) return -ENODEV;
    HIDD_PCIDriver_WriteConfigByte(pciDriver, NULL, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), where, val);
    return 0;
}
int pci_bus_write_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 val)
{
    if (!pciDriver) return -ENODEV;
    HIDD_PCIDriver_WriteConfigWord(pciDriver, NULL, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), where, val);
    return 0;
}
int pci_bus_write_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 val)
{
    if (!pciDriver) return -ENODEV;
    HIDD_PCIDriver_WriteConfigLong(pciDriver, NULL, bus->number, PCI_SLOT(devfn), PCI_FUNC(devfn), where, val);
    return 0;
}

int pcie_capability_read_word(struct pci_dev *pdev, int pos, u16 *val)
{
    if (!pdev->pcie_cap) {
        *val = 0;
        return -EINVAL;
    }
    return pci_read_config_word(pdev, pdev->pcie_cap + pos, val);
}

int pcie_capability_read_dword(struct pci_dev *pdev, int pos, u32 *val)
{
    if (!pdev->pcie_cap) {
        *val = 0;
        return -EINVAL;
    }
    return pci_read_config_dword(pdev, pdev->pcie_cap + pos, val);
}

int pcie_capability_write_word(struct pci_dev *pdev, int pos, u16 val)
{
    if (!pdev->pcie_cap)
        return -EINVAL;
    return pci_write_config_word(pdev, pdev->pcie_cap + pos, val);
}

int pcie_capability_clear_and_set_word(struct pci_dev *pdev, int pos, u16 clear, u16 set)
{
    u16 val;
    int ret = pcie_capability_read_word(pdev, pos, &val);
    if (ret)
        return ret;
    val = (val & ~clear) | set;
    return pcie_capability_write_word(pdev, pos, val);
}

int pci_enable_device(struct pci_dev *pdev)
{
    struct TagItem attrs[] = {
        { aHidd_PCIDevice_isMEM,    TRUE },
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE, 0 }
    };
    if (pdev->oopdev)
        OOP_SetAttrs((OOP_Object *)pdev->oopdev, attrs);
    return 0;
}

void pci_disable_device(struct pci_dev *pdev)
{
}

void pci_set_master(struct pci_dev *pdev)
{
    struct TagItem attrs[] = {
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE, 0 }
    };
    if (pdev->oopdev)
        OOP_SetAttrs((OOP_Object *)pdev->oopdev, attrs);
    pdev->is_busmaster = 1;
}

void pci_clear_master(struct pci_dev *pdev)
{
    pdev->is_busmaster = 0;
}

/*
 * MSI: one vector, whose hardware interrupt then replaces the INTx line
 * in pdev->irq so that request_irq() hooks the right one.
 */
int pci_enable_msi(struct pci_dev *pdev)
{
    struct TagItem vectreqs[] = {
        { tHidd_PCIVector_Min, 1 },
        { tHidd_PCIVector_Max, 1 },
        { TAG_DONE, 0 }
    };
    struct TagItem vecattrs[] = {
        { tHidd_PCIVector_Int, (IPTR)-1 },
        { TAG_DONE, 0 }
    };

    if (!pdev->oopdev)
        return -ENODEV;
    if (!HIDD_PCIDevice_ObtainVectors((OOP_Object *)pdev->oopdev, vectreqs))
        return -ENODEV;
    HIDD_PCIDevice_GetVectorAttribs((OOP_Object *)pdev->oopdev, 0, vecattrs);
    if (vecattrs[0].ti_Data == (IPTR)-1) {
        HIDD_PCIDevice_ReleaseVectors((OOP_Object *)pdev->oopdev);
        return -ENODEV;
    }
    pdev->irq = (unsigned int)vecattrs[0].ti_Data;
    pdev->msi_enabled = 1;
    return 0;
}

void pci_disable_msi(struct pci_dev *pdev)
{
    if (pdev->msi_enabled && pdev->oopdev) {
        HIDD_PCIDevice_ReleaseVectors((OOP_Object *)pdev->oopdev);
        pdev->msi_enabled = 0;
    }
}

int pci_alloc_irq_vectors(struct pci_dev *pdev, unsigned int min_vecs, unsigned int max_vecs, unsigned int flags)
{
    if ((flags & PCI_IRQ_MSI) && pci_enable_msi(pdev) == 0)
        return 1;
    if (flags & PCI_IRQ_INTX)
        return 1;
    return -ENOSPC;
}

void pci_free_irq_vectors(struct pci_dev *pdev)
{
    pci_disable_msi(pdev);
}

int pci_irq_vector(struct pci_dev *pdev, unsigned int nr)
{
    return pdev->irq;
}

int pci_enable_rom(struct pci_dev *pdev)
{
    return 0;
}

void pci_disable_rom(struct pci_dev *pdev)
{
}

void __iomem *pci_map_rom(struct pci_dev *pdev, size_t *size)
{
    IPTR rom = 0, romsize = 0;

    if (!pdev->oopdev)
        return NULL;
    OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_RomBase, &rom);
    OOP_GetAttr((OOP_Object *)pdev->oopdev, aHidd_PCIDevice_RomSize, &romsize);
    if (!rom || !romsize)
        return NULL;
    pdev->rom = rom;
    pdev->romlen = romsize;
    *size = romsize;
    return ioremap(rom, romsize);
}

void pci_unmap_rom(struct pci_dev *pdev, void __iomem *rom)
{
    iounmap(rom);
}

void __iomem *pci_platform_rom(struct pci_dev *pdev, size_t *size)
{
    return NULL;
}

/* only the one device is ever known; other lookups find nothing */
struct pci_dev *pci_get_domain_bus_and_slot(int domain, unsigned int bus, unsigned int devfn)
{
    return NULL;
}

struct pci_dev *pci_get_class(unsigned int class, struct pci_dev *from)
{
    return NULL;
}

struct pci_dev *pci_get_device(unsigned int vendor, unsigned int device, struct pci_dev *from)
{
    return NULL;
}
