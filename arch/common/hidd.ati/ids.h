#ifndef IDS_H_
#define IDS_H_
/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI ids for ATI
    Lang: English
*/

#include <exec/types.h>
#include "ati.h"

struct ATIDevice {
    UWORD VendorID, ProductID __attribute__((packed));
    CardType Type;
    BOOL (*Init)(struct staticdata*);
    BOOL masked_check;
};

extern const struct ATIDevice support[] __attribute__((section(".pci.supids")));

#endif /*IDS_H_*/
