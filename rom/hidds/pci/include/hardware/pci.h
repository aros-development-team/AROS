#ifndef HARDWARE_PCI_H
#define HARDWARE_PCI_H

/* PCI class ID */
#define PCI_CLASS_MASSSTORAGE	        0x01

/* Valid subclasses, taken from http://pci-ids.ucw.cz/read/PD/01 */
#define PCI_SUBCLASS_SCSI	        0x0
#define PCI_SUBCLASS_IDE	        0x1
#define PCI_SUBCLASS_FLOPPY	        0x2
#define PCI_SUBCLASS_IPI	        0x3
#define PCI_SUBCLASS_RAID	        0x4
#define PCI_SUBCLASS_ATA	        0x5
#define PCI_SUBCLASS_SATA	        0x6
#define PCI_SUBCLASS_SAS	        0x7
#define PCI_SUBCLASS_NVM	        0x8
#define PCI_SUBCLASS_MASSSTORAGE	0x80

/* PCI Configspace offsets */
#define PCICS_VENDOR        0x00
#define PCICS_PRODUCT       0x02
#define PCICS_COMMAND       0x04
#define PCICS_STATUS        0x06
#define PCICS_REVISION      0x08
#define PCICS_PROGIF        0x09
#define PCICS_SUBCLASS      0x0a
#define PCICS_CLASS         0x0b
#define PCICS_CACHELS       0x0c
#define PCICS_LATENCY       0x0d
#define PCICS_HEADERTYPE    0x0e
#define PCICS_BIST          0x0f
#define PCICS_BAR0          0x10
#define PCICS_BAR1          0x14
#define PCICS_BAR2          0x18
#define PCICS_BAR3          0x1c
#define PCICS_BAR4          0x20
#define PCICS_BAR5          0x24
#define PCICS_CARDBUS_CIS   0x28
#define PCICS_SUBVENDOR     0x2c
#define PCICS_SUBSYSTEM     0x2e
#define PCICS_EXPROM_BASE   0x30
#define PCICS_CAP_PTR       0x34
#define PCICS_INT_LINE      0x3c
#define PCICS_INT_PIN       0x3d
#define PCICS_MIN_GNT       0x3e
#define PCICS_MAX_LAT       0x3f

/* PCI Headertypes */
#define PCIHT_MASK          0x7f
#define PCIHT_MULTIFUNC     0x80

#define PCIHT_NORMAL        0x00
#define PCIHT_BRIDGE        0x01
#define PCIHT_CARDBUS       0x02

/* PCI Command register bits */
#define PCICMB_IODECODE     0
#define PCICMB_MEMDECODE    1
#define PCICMB_BUSMASTER    2
#define PCICMB_SPECIAL      3
#define PCICMB_INVALIDATE   4
#define PCICMB_VGASNOOP     5
#define PCICMB_PARITY       6
#define PCICMB_STEPPING     7
#define PCICMB_SERR         8
#define PCICMB_FASTB2B      9

#define PCICMF_IODECODE     (1 << PCICMB_IODECODE)
#define PCICMF_MEMDECODE    (1 << PCICMB_MEMDECODE)
#define PCICMF_BUSMASTER    (1 << PCICMB_BUSMASTER)
#define PCICMF_SPECIAL      (1 << PCICMB_SPECIAL)
#define PCICMF_INVALIDATE   (1 << PCICMB_INVALIDATE)
#define PCICMF_VGASNOOP     (1 << PCICMB_VGASNOOP)
#define PCICMF_PARITY       (1 << PCICMB_PARITY)
#define PCICMF_STEPPING     (1 << PCICMB_STEPPING)
#define PCICMF_SERR         (1 << PCICMB_SERR)
#define PCICMF_FASTB2B      (1 << PCICMB_FASTB2B)

/* PCI Status register bits */
#define PCISTB_INTERRUPT_STATUS 3       /* might be AHCI specific */
#define PCISTB_CAPABILITES      4
#define PCISTB_66MHZ            5
#define PCISTB_FASTB2B          7
#define PCISTB_PARITY           8
#define PCISTB_SIG_TGT_ABORT    11
#define PCISTB_REC_TGT_ABORT    12
#define PCISTB_REC_MAS_ABORT    13
#define PCISTB_SIG_SYSERR       14
#define PCISTB_PARITYERR        15

#define PCISTF_INTERRUPT_STATUS (1 << PCISTB_INTERRUPT_STATUS) 
#define PCISTF_CAPABILITIES     (1 << PCISTB_CAPABILITES)
#define PCISTF_66MHZ            (1 << PCISTB_66MHZ)
#define PCISTF_FASTB2B          (1 << PCISTB_FASTB2B)
#define PCISTF_PARITY           (1 << PCISTB_PARITY)
#define PCISTF_SIG_TGT_ABORT    (1 << PCISTB_SIG_TGT_ABORT)
#define PCISTF_REC_TGT_ABORT    (1 << PCISTB_REC_TGT_ABORT)
#define PCISTF_REC_MAS_ABORT    (1 << PCISTB_REC_MAS_ABORT)
#define PCISTF_SIG_SYSERR       (1 << PCISTB_SIG_SYSERR)
#define PCISTF_PARITYERR        (1 << PCISTB_PARITYERR)

#define PCIST_DEVSEL_MASK   0x600
#define PCIST_DEVSEL_FAST   0x000
#define PCIST_DEVSEL_MEDIUM 0x200
#define PCIST_DEVSEL_SLOW   0x400

/* PCI BIST register */
#define PCIBSB_START            6
#define PCIBSB_CAPABLE          7

#define PCIBSF_START            (1 << PCIBSB_START)
#define PCIBSF_CAPABLE          (1 << PCIBSB_CAPABLE)

#define PCIBS_CODEMASK          0x0f

/* PCI BaseAddressRegister defines */
#define PCIBAR_MASK_TYPE        0x01
#define PCIBAR_TYPE_MMAP        0x00
#define PCIBAR_TYPE_IO          0x01
#define PCIBAR_MASK_MEM         0xfffffff0
#define PCIBAR_MASK_IO          0xfffffffc

#define PCIBAR_MEMTYPE_MASK     0x06
#define PCIBAR_MEMTYPE_32BIT    0x00
#define PCIBAR_MEMTYPE_64BIT    0x04

#define PCIBARB_PREFETCHABLE    3
#define PCIBARF_PREFETCHABLE    (1 << PCIBARB_PREFETCHABLE)

/*
 * PCI-to-PCI bridge header defines
 * First 16 bytes are the same as normal PCI dev
 * Use either PCICS_ or PCIBR_ prefix
 */
#define PCIBR_VENDOR        PCICS_VENDOR
#define PCIBR_PRODUCT       PCICS_PRODUCT
#define PCIBR_COMMAND       PCICS_COMMAND
#define PCIBR_STATUS        PCICS_STATUS
#define PCIBR_REVISION      PCICS_REVISION
#define PCIBR_PROGIF        PCICS_PROGIF
#define PCIBR_SUBCLASS      PCICS_SUBCLASS
#define PCIBR_CLASS         PCICS_CLASS
#define PCIBR_CACHELS       PCICS_CACHELS
#define PCIBR_LATENCY       PCICS_LATENCY
#define PCIBR_HEADERTYPE    PCICS_HEADERTYPE
#define PCIBR_BIST          PCICS_BIST
#define PCIBR_BAR0          0x10
#define PCIBR_BAR1          0x14
#define PCIBR_PRIBUS        0x18
#define PCIBR_SECBUS        0x19
#define PCIBR_SUBBUS        0x1a
#define PCIBR_SECLATENCY    0x1b
#define PCIBR_IOBASE        0x1c
#define PCIBR_IOLIMIT       0x1d
#define PCIBR_SECSTATUS     0x1e
#define PCIBR_MEMBASE       0x20
#define PCIBR_MEMLIMIT      0x22
#define PCIBR_PREFETCHBASE  0x24
#define PCIBR_PREFETCHLIMIT 0x26
#define PCIBR_PREBASEUPPER  0x28
#define PCIBR_PRELIMITUPPER 0x2c
#define PCIBR_IOBASEUPPER   0x30
#define PCIBR_IOLIMITUPPER  0x32
#define PCIBR_CAPPTR        0x34
#define PCIBR_EXPROMBASE    0x38
#define PCIBR_INT_LINE                  0x3c
#define PCIBR_INT_PIN                   0x3d
#define PCIBR_CONTROL                   0x3e

#define PCICTRLB_ISAENABLE              2
#define PCICTRLB_VGAENABLE              3

#define PCICTRLF_ISAENABLE              (1 << PCICTRLB_ISAENABLE)
#define PCICTRLF_VGAENABLE              (1 << PCICTRLB_VGAENABLE)

/* PCI capabilities */
#define PCICAP_POWER_MANAGEMENT         0x01
#define PCICAP_AGP                      0x02
#define PCICAP_VITAL_PRODUCT_DATA       0x03
#define PCICAP_SLOT_ID                  0x04
#define PCICAP_MSI                      0x05
#define PCICAP_CPCI_HOT_SWAP            0x06
#define PCICAP_PCIX                     0x07
#define PCICAP_HYPER_TRANSPORT          0x08
#define PCICAP_VENDOR_SPECIFIC          0x09
#define PCICAP_DEBUG_PORT               0x0a
#define PCICAP_CPCI_CR                  0x0b
#define PCICAP_HOT_PLUG_CONTROLLER      0x0c
#define PCICAP_SSVPID                   0x0d
#define PCICAP_AGP3                     0x0e
#define PCICAP_PCIE                     0x10
#define PCICAP_MSIX                     0x11
#define PCICAP_ADVANCED_FEATURES        0x13

/* MSI capability defines */
#define PCIMSI_FLAGS                    0                                /* MSI Control Flags Word */
#define PCIMSI_NXT                      2                                /* Next ID Byte */
#define PCIMSI_CAP                      3                                /* MSI Capability Flags Byte */
#define PCIMSI_ADDRESSLO                4                                /* MAR Lower 32 bits */
#define PCIMSI_DATA32                   10                               /* (32-bit) Data Word */
#define PCIMSI_ADDRESSHI                8                                /* MAR Upper 32 bits (PCIMSIF_64BIT) */
#define PCIMSI_DATA64                   14                               /* (64-bit) Data Word */

#define PCIMSIB_ENABLE                  0                               /* MSI feature enable(d) */
#define PCIMSIF_ENABLE                  (1 << PCIMSIB_ENABLE)
#define PCIMSIB_MMC                     1                               /* Multi-message Capable */
#define PCIMSIB_MMEN                    4                               /* Multi-message Enable */
#define PCIMSIB_64BIT                   7                               /* 64-bit addresses */
#define PCIMSIF_64BIT                   (1 << PCIMSIB_64BIT)
#define PCIMSIB_MASKBIT                 8                               /* 64-bit mask bits */
#define PCIMSIF_MASKBIT                 (1 << PCIMSIB_MASKBIT)

#define PCIMSIF_MMC_MASK                (0x7 << PCIMSIB_MMC)            /* Maximum available queue size */
#define PCIMSIF_MMEN_MASK               (0x7 << PCIMSIB_MMEN)           /* Configured message queue size */

/* MSIX capability defines */
#define PCIMSIX_FLAGS                   2
#define PCIMSIX_TABLE                   4
#define PCIMSIX_PBA                     8
#define PCI_CAP_MSIX_SIZEOF             12                               /* size of MSIX registers */
#define PCIMSIX_ENTRY_SIZE              16

#define PCIMSIXB_MASKALL                14
#define PCIMSIXF_MASKALL                (1 << PCIMSIXB_MASKALL)
#define PCIMSIXB_ENABLE                 15
#define PCIMSIXF_ENABLE                 (1 << PCIMSIXB_ENABLE)
#define PCIMSIXF_QSIZE                  0x7FF
#define PCIMSIXF_BIRMASK                (0x7 << 0)

#define PCIMSIX_ENTRY_LOWER_ADDR        0
#define PCIMSIX_ENTRY_UPPER_ADDR        4
#define PCIMSIX_ENTRY_DATA              8
#define PCIMSIX_ENTRY_VECTOR_CTRL       12

#define PCIMSIX_ENTRY_CTRL_MASKBIT      1

#endif /* !HARDWARE_PCI_H */