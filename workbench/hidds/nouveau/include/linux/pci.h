/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_PCI_H_
#define _LINUX_PCI_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/pci_ids.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/slab.h>

enum pci_bus_speed {
    PCIE_SPEED_2_5GT = 0x14,
    PCIE_SPEED_5_0GT = 0x15,
    PCIE_SPEED_8_0GT = 0x16,
    PCIE_SPEED_16_0GT = 0x17,
    PCIE_SPEED_32_0GT = 0x18,
    PCIE_SPEED_64_0GT = 0x19,
    PCI_SPEED_UNKNOWN = 0xff,
};
struct pci_bus {
    unsigned char number;
    unsigned char max_bus_speed;
    unsigned char cur_bus_speed;
    struct pci_bus *parent;
    struct pci_dev *self;
    int domain_nr;
};

/*
 * The AROS device sits on the pci.hidd; oopdev is the PCIDevice object
 * every configuration or mapping request goes through.
 */
struct pci_dev {
    struct device dev;
    struct pci_bus *bus;
    struct pci_bus bus_storage;
    unsigned int devfn;
    unsigned short vendor;
    unsigned short device;
    unsigned short subsystem_vendor;
    unsigned short subsystem_device;
    unsigned int class;
    u8 revision;
    u8 hdr_type;
    unsigned int irq;
    unsigned int is_pcie:1;
    unsigned int msi_enabled:1;
    unsigned int is_busmaster:1;
    unsigned int isAGP:1;
    unsigned int isPCIE:1;
    void *oopdev;
    char name[32];
    IPTR rom;
    size_t romlen;
    u32 pcie_cap;
    u8 pm_cap;
    void *driver_data;
};

struct pci_device_id {
    u32 vendor, device;
    u32 subvendor, subdevice;
    u32 class, class_mask;
    kernel_ulong_t driver_data;
};

struct pci_driver {
    const char *name;
    const struct pci_device_id *id_table;
    int (*probe)(struct pci_dev *dev, const struct pci_device_id *id);
    void (*remove)(struct pci_dev *dev);
    void (*shutdown)(struct pci_dev *dev);
    const struct dev_pm_ops *pm;
    struct device_driver driver;
};

#define PCI_ANY_ID                  (~0)
#define PCI_DEVICE(vend, dev)       .vendor = (vend), .device = (dev), .subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
#define PCI_VDEVICE(vend, dev)      .vendor = PCI_VENDOR_ID_##vend, .device = (dev), .subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID, 0, 0
#define PCI_DEVICE_CLASS(c, m)      .class = (c), .class_mask = (m), .vendor = PCI_ANY_ID, .device = PCI_ANY_ID, .subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
#define PCI_DEVFN(slot, func)       ((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)             (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)             ((devfn) & 0x07)
#define PCI_BUS_NUM(x)              (((x) >> 8) & 0xff)
#define PCI_STD_NUM_BARS            6
#define PCI_ROM_RESOURCE            6
#define PCI_NUM_RESOURCES           17
#define IORESOURCE_IO               0x00000100
#define IORESOURCE_MEM              0x00000200
#define IORESOURCE_MEM_64           0x00100000
#define IORESOURCE_PREFETCH         0x00002000
#define IORESOURCE_ROM_SHADOW       0x00000000
#define IORESOURCE_UNSET            0x20000000
#define pci_domain_nr(bus)          ((bus)->domain_nr)
#define pci_dev_id(pdev)            ((u16)((((pdev)->bus->number) << 8) | (pdev)->devfn))
#define to_pci_dev(d)               container_of(d, struct pci_dev, dev)
#define pci_name(pdev)              ((const char *)(pdev)->name)
#define pci_get_drvdata(pdev)       dev_get_drvdata(&(pdev)->dev)
#define pci_set_drvdata(pdev, d)    dev_set_drvdata(&(pdev)->dev, d)
#define pci_dev_get(pdev)           (pdev)
#define pci_dev_put(pdev)           do { } while (0)
#define pci_is_root_bus(bus)        ((bus)->parent == NULL)
#define pci_upstream_bridge(pdev)   ((struct pci_dev *)NULL)
#define pci_pcie_type(pdev)         (0)
#define pci_is_pcie(pdev)           ((pdev)->is_pcie)
#define pci_is_bridge(pdev)         (0)
#define pci_is_thunderbolt_attached(pdev) (0)
#define pci_is_vga(pdev)            (((pdev)->class >> 8) == PCI_CLASS_DISPLAY_VGA)
#define pci_find_capability(pdev, c) (0)
#define pci_find_ext_capability(pdev, c) (0)
#define pci_pcie_cap(pdev)          ((pdev)->pcie_cap)
#define pci_msi_enabled()           (1)
#define pci_dev_msi_enabled(pdev)   ((pdev)->msi_enabled)
#define pci_channel_offline(pdev)   (0)
#define pci_ats_supported(pdev)     (0)
#define pcie_get_speed_cap(pdev)    (0)
#define pcie_get_width_cap(pdev)    (0)
#define pci_read_config_word_p(pdev, w, v) pci_read_config_word(pdev, w, v)

resource_size_t pci_resource_start(struct pci_dev *pdev, unsigned int bar);
resource_size_t pci_resource_len(struct pci_dev *pdev, unsigned int bar);
static inline resource_size_t pci_resource_end(struct pci_dev *pdev, unsigned int bar)
{
    return pci_resource_start(pdev, bar) + pci_resource_len(pdev, bar) - 1;
}
unsigned long pci_resource_flags(struct pci_dev *pdev, unsigned int bar);
/* bus -> CPU translation of a BAR address (no mapping is made) */
void *pci_resource_cpu_addr(resource_size_t busaddr);

int pci_read_config_byte(struct pci_dev *pdev, int where, u8 *val);
int pci_read_config_word(struct pci_dev *pdev, int where, u16 *val);
int pci_read_config_dword(struct pci_dev *pdev, int where, u32 *val);
int pci_write_config_byte(struct pci_dev *pdev, int where, u8 val);
int pci_write_config_word(struct pci_dev *pdev, int where, u16 val);
int pci_write_config_dword(struct pci_dev *pdev, int where, u32 val);
int pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 *val);
int pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 *val);
int pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 *val);
int pci_bus_write_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 val);
int pci_bus_write_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 val);
int pci_bus_write_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 val);
int pcie_capability_read_word(struct pci_dev *pdev, int pos, u16 *val);
int pcie_capability_read_dword(struct pci_dev *pdev, int pos, u32 *val);
int pcie_capability_write_word(struct pci_dev *pdev, int pos, u16 val);
int pcie_capability_clear_and_set_word(struct pci_dev *pdev, int pos, u16 clear, u16 set);
#define pcie_capability_set_word(pdev, pos, set)     pcie_capability_clear_and_set_word(pdev, pos, 0, set)
#define pcie_capability_clear_word(pdev, pos, clear) pcie_capability_clear_and_set_word(pdev, pos, clear, 0)

int  pci_enable_device(struct pci_dev *pdev);
void pci_disable_device(struct pci_dev *pdev);
void pci_set_master(struct pci_dev *pdev);
void pci_clear_master(struct pci_dev *pdev);
int  pci_enable_msi(struct pci_dev *pdev);
void pci_disable_msi(struct pci_dev *pdev);
int  pci_alloc_irq_vectors(struct pci_dev *pdev, unsigned int min_vecs, unsigned int max_vecs, unsigned int flags);
void pci_free_irq_vectors(struct pci_dev *pdev);
int  pci_irq_vector(struct pci_dev *pdev, unsigned int nr);
#define PCI_IRQ_INTX            (1 << 0)
#define PCI_IRQ_LEGACY          PCI_IRQ_INTX
#define PCI_IRQ_MSI             (1 << 1)
#define PCI_IRQ_MSIX            (1 << 2)
#define PCI_IRQ_ALL_TYPES       (PCI_IRQ_INTX | PCI_IRQ_MSI | PCI_IRQ_MSIX)
int  pci_enable_rom(struct pci_dev *pdev);
void pci_disable_rom(struct pci_dev *pdev);
void __iomem *pci_map_rom(struct pci_dev *pdev, size_t *size);
void pci_unmap_rom(struct pci_dev *pdev, void __iomem *rom);
void __iomem *pci_platform_rom(struct pci_dev *pdev, size_t *size);
struct pci_dev *pci_get_domain_bus_and_slot(int domain, unsigned int bus, unsigned int devfn);
#define pci_get_bus_and_slot(bus, devfn) pci_get_domain_bus_and_slot(0, bus, devfn)
struct pci_dev *pci_get_class(unsigned int class, struct pci_dev *from);
struct pci_dev *pci_get_device(unsigned int vendor, unsigned int device, struct pci_dev *from);
#define pci_save_state(pdev)            (0)
#define pci_restore_state(pdev)         do { } while (0)
#define pci_set_power_state(pdev, s)    (0)
#define pci_ignore_hotplug(pdev)        do { } while (0)
#define pci_wake_from_d3(pdev, e)       (0)
#define pci_register_driver(d)          (0)
#define pci_unregister_driver(d)        do { } while (0)
#define pci_request_regions(pdev, n)    (0)
#define pci_release_regions(pdev)       do { } while (0)
#define pci_request_region(pdev, b, n)  (0)
#define pci_release_region(pdev, b)     do { } while (0)
#define pci_resource_to_user(a, b, c, d, e) do { } while (0)
#define pci_intx(pdev, e)               do { } while (0)
#define pcie_reset_flr(pdev, p)         (-ENOTTY)
#define pci_reset_function(pdev)        (-ENOTTY)
#define pci_d3cold_disable(pdev)        do { } while (0)
#define pci_d3cold_enable(pdev)         do { } while (0)
#define pci_dev_present(ids)            (0)
#define pci_pme_capable(pdev, s)        (0)
#define pci_disable_link_state(pdev, s) (0)
#define pci_enable_atomic_ops_to_root(pdev, c) (-EINVAL)
#define pcibios_align_resource          NULL
#define pci_dev_run_wake(pdev)          (0)
#define pci_vpd_find_tag(a, b, c)       (-ENOENT)
#define pci_read_vpd(a, b, c, d)        (-ENOENT)
#define pci_domain_nr_of(pdev)          0
#define pci_dev_is_disconnected(pdev)   (0)
#define pci_status_get_and_clear_errors(pdev) (0)
#define pci_msix_can_alloc_dyn(pdev)    (0)
#define pci_msix_alloc_irq_at(pdev, i, a) ((struct msi_map){ .index = -1, .virq = -1 })
struct msi_map { int index; int virq; };
#define PCI_D0                          0
#define PCI_D3hot                       3
#define PCI_D3cold                      4
#define PCI_UNKNOWN                     5
typedef int pci_power_t;
#define PCI_ROM_ADDRESS                 0x30
#define PCI_ROM_ADDRESS_ENABLE          0x01
#define PCI_COMMAND                     0x04
#define PCI_COMMAND_MEMORY              0x2
#define PCI_COMMAND_MASTER              0x4
#define PCI_COMMAND_IO                  0x1
#define PCI_STATUS                      0x06
#define PCI_REVISION_ID                 0x08
#define PCI_CLASS_REVISION              0x08
#define PCI_CLASS_DEVICE                0x0a
#define PCI_HEADER_TYPE                 0x0e
#define PCI_BASE_ADDRESS_0              0x10
#define PCI_BASE_ADDRESS_1              0x14
#define PCI_BASE_ADDRESS_2              0x18
#define PCI_BASE_ADDRESS_3              0x1c
#define PCI_BASE_ADDRESS_4              0x20
#define PCI_BASE_ADDRESS_5              0x24
#define PCI_SUBSYSTEM_VENDOR_ID         0x2c
#define PCI_SUBSYSTEM_ID                0x2e
#define PCI_CAPABILITY_LIST             0x34
#define PCI_INTERRUPT_LINE              0x3c
#define PCI_INTERRUPT_PIN               0x3d
#define PCI_CAP_ID_PM                   0x01
#define PCI_CAP_ID_AGP                  0x02
#define PCI_CAP_ID_MSI                  0x05
#define PCI_CAP_ID_EXP                  0x10
#define PCI_CAP_ID_MSIX                 0x11
#define PCI_EXP_FLAGS                   0x02
#define PCI_EXP_FLAGS_TYPE              0x00f0
#define PCI_EXP_TYPE_ENDPOINT           0x0
#define PCI_EXP_TYPE_LEG_END            0x1
#define PCI_EXP_TYPE_ROOT_PORT          0x4
#define PCI_EXP_TYPE_UPSTREAM           0x5
#define PCI_EXP_TYPE_DOWNSTREAM         0x6
#define PCI_EXP_DEVCAP                  0x04
#define PCI_EXP_DEVCTL                  0x08
#define PCI_EXP_DEVCTL_READRQ           0x7000
#define PCI_EXP_DEVCTL_PAYLOAD          0x00e0
#define PCI_EXP_LNKCAP                  0x0c
#define PCI_EXP_LNKCAP_SLS              0x0000000f
#define PCI_EXP_LNKCAP_MLW              0x000003f0
#define PCI_EXP_LNKCTL                  0x10
#define PCI_EXP_LNKCTL_ASPMC            0x0003
#define PCI_EXP_LNKCTL_ASPM_L0S         0x0001
#define PCI_EXP_LNKCTL_ASPM_L1          0x0002
#define PCI_EXP_LNKSTA                  0x12
#define PCI_EXP_LNKSTA_CLS              0x000f
#define PCI_EXP_LNKSTA_CLS_2_5GB        0x0001
#define PCI_EXP_LNKSTA_CLS_5_0GB        0x0002
#define PCI_EXP_LNKSTA_CLS_8_0GB        0x0003
#define PCI_EXP_LNKSTA_NLW              0x03f0
#define PCI_EXP_LNKSTA_NLW_SHIFT        4
#define PCI_EXP_LNKCAP2                 0x2c
#define PCI_EXP_LNKCTL2                 0x30
#define PCI_EXP_LNKCTL2_TLS             0x000f
#define PCI_EXP_LNKCTL2_TLS_2_5GT       0x0001
#define PCI_EXP_LNKCTL2_TLS_5_0GT       0x0002
#define PCI_EXP_LNKCTL2_TLS_8_0GT       0x0003
#define PCI_EXP_LNKCTL2_TLS_16_0GT      0x0004
#define PCI_EXP_LNKCTL2_TLS_32_0GT      0x0005
#define PCI_EXP_LNKCTL2_TLS_64_0GT      0x0006
#define PCI_EXP_LNKCTL2_HASD            0x0020
#define PCI_EXP_LNKCAP2_SLS_2_5GB       0x00000002
#define PCI_EXP_LNKCAP2_SLS_5_0GB       0x00000004
#define PCI_EXP_LNKCAP2_SLS_8_0GB       0x00000008
#define PCI_EXP_LNKCAP2_SLS_16_0GB      0x00000010
#define PCI_EXP_LNKCAP2_SLS_32_0GB      0x00000020
#define PCI_EXP_LNKCAP2_SLS_64_0GB      0x00000040
#define PCI_EXP_SLTCAP                  0x14
#define PCI_EXP_SLTCTL                  0x18
#define PCI_EXP_SLTSTA                  0x1a
#define PCI_EXP_RTCTL                   0x1c
#define PCI_EXP_DEVCAP2                 0x24
#define PCI_EXP_DEVCTL2                 0x28
#define PCI_EXP_DEVCTL2_ATOMIC_REQ      0x0040
#define PCI_EXP_DEVCAP2_ATOMIC_COMP32   0x00000080
#define PCI_EXP_DEVCAP2_ATOMIC_COMP64   0x00000100
#define PCI_EXP_DEVCAP2_ATOMIC_COMP128  0x00000200
#define PCI_AGP_STATUS                  4
#define PCI_AGP_COMMAND                 8
#define PCI_AGP_STATUS_RQ_MASK          0xff000000
#define PCI_AGP_STATUS_SBA              0x0200
#define PCI_AGP_STATUS_64BIT            0x0020
#define PCI_AGP_STATUS_FW               0x0010
#define PCI_AGP_STATUS_RATE4            0x0004
#define PCI_AGP_STATUS_RATE2            0x0002
#define PCI_AGP_STATUS_RATE1            0x0001
#define PCI_AGP_COMMAND_RQ_MASK         0xff000000
#define PCI_AGP_COMMAND_SBA             0x0200
#define PCI_AGP_COMMAND_AGP             0x0100
#define PCI_AGP_COMMAND_64BIT           0x0020
#define PCI_AGP_COMMAND_FW              0x0010
#define PCI_AGP_COMMAND_RATE4           0x0004
#define PCI_AGP_COMMAND_RATE2           0x0002
#define PCI_AGP_COMMAND_RATE1           0x0001
#define PCI_CLASS_DISPLAY_VGA           0x0300
#define PCI_CLASS_DISPLAY_3D            0x0302
#define PCI_CLASS_BRIDGE_HOST           0x0600
#define PCI_CLASS_BRIDGE_PCI            0x0604
#define PCI_BASE_CLASS_DISPLAY          0x03
#define PCI_BASE_CLASS_BRIDGE           0x06
#define PCIE_LINK_STATE_L0S             1
#define PCIE_LINK_STATE_L1              2
#define PCIE_LINK_STATE_CLKPM           4
#define PCIE_LNK_WIDTH_UNKNOWN          0
#define PCI_MSI_FLAGS                   0x02
#define PCI_MSI_FLAGS_ENABLE            0x0001
#define PCI_MSIX_FLAGS                  0x02
#define PCI_MSIX_FLAGS_ENABLE           0x8000

#endif /* _LINUX_PCI_H_ */
