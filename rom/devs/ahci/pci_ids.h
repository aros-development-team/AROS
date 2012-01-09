#define PCI_VENDOR_INTEL                0x8086
#define PCI_VENDOR_VIATECH              0x1106
#define PCI_PRODUCT_VIATECH_VT8251_SATA 0x3287
#define PCI_VENDOR_ATI                  0x1002
#define PCI_PRODUCT_ATI_SB600_SATA      0x4380
#define PCI_VENDOR_NVIDIA               0x10de
#define PCI_PRODUCT_NVIDIA_MCP65_AHCI_2 0x0448
#define PCI_PRODUCT_NVIDIA_MCP67_AHCI_1 0x0560
#define PCI_PRODUCT_NVIDIA_MCP77_AHCI_5 0x0759

#define PCIR_VENDOR     0x00
#define PCIR_DEVICE     0x02
#define PCIR_COMMAND    0x04
#define PCIR_REVID      0x08
#define PCIR_PROGIF     0x09
#define PCIR_SUBCLASS   0x0a
#define PCIR_CLASS      0x0b
#define PCIR_BAR(x)     (0x10 + (x) * 4)

#define PCIC_STORAGE                    0x01

#define PCIS_STORAGE_IDE                0x01
#define PCIS_STORAGE_SATA               0x06
#define PCIP_STORAGE_SATA_AHCI_1_0              0x01
