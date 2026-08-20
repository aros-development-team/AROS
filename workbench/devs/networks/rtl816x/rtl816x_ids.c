/*
 * $Id$
 */

#include "rtl816x.h"

/* Entries are grouped by vendor - the enumerator relies on this to visit
   each vendor once */
const struct card_def cards[] =
{
    { PCI_VENDOR_ID_REALTEK,    0x8129, RTL_CFG_0 },
    { PCI_VENDOR_ID_REALTEK,    0x8136, RTL_CFG_2 },
    { PCI_VENDOR_ID_REALTEK,    0x8161, RTL_CFG_1 },
    { PCI_VENDOR_ID_REALTEK,    0x8167, RTL_CFG_0 },
    { PCI_VENDOR_ID_REALTEK,    0x8168, RTL_CFG_1 },
    { PCI_VENDOR_ID_REALTEK,    0x8169, RTL_CFG_0 },
    { PCI_VENDOR_ID_DLINK,      0x4300, RTL_CFG_0 },
    { PCI_VENDOR_ID_AT,         0xc107, RTL_CFG_0 },
    { PCI_VENDOR_ID_USROBOTICS, 0x0116, RTL_CFG_0 },
    { PCI_VENDOR_ID_LINKSYS,    0x1032, RTL_CFG_0 },
    { 0x0001,                   0x8168, RTL_CFG_2 },
    { 0, 0, 0 }
};
