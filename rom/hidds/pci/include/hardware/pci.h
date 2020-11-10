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

/* Configuration space registers */
#define IDE_IO_CFG                      0x54

#define IOCFG_SCR1                      (1 << 7) /* Secondary 1 cable report */
#define IOCFG_SCR0                      (1 << 6) /* Secondary 0 cable report */
#define IOCFG_PCR1                      (1 << 5) /* Primary 1 cable report   */
#define IOCFG_PCR0                      (1 << 4) /* Primary 0 cable report   */

#endif /* !HARDWARE_PCI_H */