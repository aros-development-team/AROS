/* (c) 2020 */

#define PCIR_VENDOR                     0x00
#define PCIR_DEVICE                     0x02
#define PCIR_COMMAND                    0x04
#define PCIR_REVID                      0x08
#define PCIR_PROGIF                     0x09
#define PCIR_SUBCLASS                   0x0a
#define PCIR_CLASS                      0x0b
#define PCIR_BAR(x)                     (0x10 + (x) * 4)

#define PCIC_STORAGE                    0x01

#define PCIS_STORAGE_IDE                0x01
#define PCIS_STORAGE_SATA               0x06

#define PCIP_STORAGE_SATA_AHCI_1_0      0x01

/* The PCI ID's need to match the expected version from DragonflyBSD! */
#define PCI_VENDOR_ATI                          0x1002
#define PCI_PRODUCT_ATI_SB600_SATA              0x4380		/* SB600 SATA */
#define PCI_PRODUCT_ATI_SB700_IDE               0x4390
#define PCI_PRODUCT_ATI_SB700_AHCI              0x4391		/* SB700 AHCI */

#define PCI_VENDOR_INTEL                        0x8086

#define	PCI_VENDOR_MARVELL	                0x11ab		/* Marvell */
#define	PCI_PRODUCT_MARVELL_88SE6121	        0x6121		/* 88SE6121 SATA II Controller */
#define	PCI_PRODUCT_MARVELL_88SE6145	        0x6145		/* 88SE6145 SATA II PCI-E Controller */

#define PCI_VENDOR_NVIDIA                       0x10de
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_1	        0x044c		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_2	        0x044d		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_3	        0x044e		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_4	        0x044f		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_5	        0x045c		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_6	        0x045d		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_7	        0x045e		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP65_AHCI_8	        0x045f		/* MCP65 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP67_AHCI_1	        0x0554		/* MCP67 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP79_AHCI_1	        0x0ab8		/* MCP79 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP79_AHCI_9	        0x0ab9		/* MCP79 AHCI */
#define	PCI_PRODUCT_NVIDIA_MCP77_AHCI_5	        0x0ad4		/* MCP77 AHCI */

#define PCI_VENDOR_VIATECH                      0x1106
#define PCI_PRODUCT_VIATECH_VT8251_SATA         0x3287
