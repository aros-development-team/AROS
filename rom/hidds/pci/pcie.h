#ifndef _PCIE_H
#define _PCIE_H

/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    PCI Express Configspace offsets
*/
#define PCIECS_VENDOR        0x00
#define PCIECS_PRODUCT       0x02
#define PCIECS_COMMAND       0x04
#define PCIECS_STATUS        0x06
#define PCIECS_REVISION      0x08
#define PCIECS_PROGIF        0x09
#define PCIECS_SUBCLASS      0x0a
#define PCIECS_CLASS         0x0b
#define PCIECS_CACHELS       0x0c
#define PCIECS_LATENCY       0x0d
#define PCIECS_HEADERTYPE    0x0e
#define PCIECS_BIST          0x0f
#define PCIECS_BAR0          0x10
#define PCIECS_BAR1          0x14
#define PCIECS_BAR2          0x18
#define PCIECS_BAR3          0x1c
#define PCIECS_BAR4          0x20
#define PCIECS_BAR5          0x24
#define PCIECS_CARDBUS_CIS   0x28
#define PCIECS_SUBVENDOR     0x2c
#define PCIECS_SUBSYSTEM     0x2e
#define PCIECS_EXPROM_BASE   0x30
#define PCIECS_CAP_PTR       0x34
#define PCIECS_INT_LINE      0x3c
#define PCIECS_INT_PIN       0x3d
#define PCIECS_MIN_GNT       0x3e
#define PCIECS_MAX_LAT       0x3f

/*
    PCI Express capability structure, rev. 3.0
*/
#define PCIECS_CAPID        0x00
#define PCIECS_NEXTCAP      0x01
#define PCIECS_PCIECAP      0x02
#define PCIECS_DEVCAP       0x04
#define PCIECS_DEVCTL       0x08
#define PCIECS_DEVSTS       0x0A
#define PCIECS_LINKCAP      0x0C
#define PCIECS_LINKCTL      0x10
#define PCIECS_LINKSTS      0x12
#define PCIECS_SLOTCAP      0x14
#define PCIECS_SLOTCTL      0x18
#define PCIECS_SLOTSTS      0x1A
#define PCIECS_ROOTCTL      0x1C
#define PCIECS_ROOTSTS      0x20    /* rev 1.0 */
#define PCIECS_DEVCAP2      0x24
#define PCIECS_DEVCTL2      0x28
#define PCIECS_DEVSTS2      0x2A
#define PCIECS_LINKCAP2     0x2C
#define PCIECS_LINKCTL2     0x30
#define PCIECS_LINKSTS2     0x32
#define PCIECS_SLOTCAP2     0x34
#define PCIECS_SLOTCTL2     0x38
#define PCIECS_SLOTSTS2     0x3A    /* rev 3.0 */

/*
    PCI Express capabilities, PCIECS_PCIECAP (incomplete)
*/
#define PCIECAP_VER_MASK    0x0F
#define PCIECAP_VER_10      0x01     /* PCIe spec 1.0 */
#define PCIECAP_VER_20      0x02     /* PCIe spec 2.0 */

/*
    PCI Express extended capabilities
*/
#define	PCIEECAP_AER        0x0001  /* Advanced Error Handling */
#define	PCIEECAP_VC         0x0002  /* Virtual Channel */
#define	PCIEECAP_SER        0x0003  /* Serial Number */
#define	PCIEECAP_PWR_BUDGET 0x0004  /* Power Budgeting */

#endif /* _PCIE_H */

