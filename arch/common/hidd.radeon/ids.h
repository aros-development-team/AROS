#ifndef IDS_H_
#define IDS_H_
/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id: ids.h 24110 2006-02-25 07:12:37 +0100 (Sat, 25 Feb 2006) schulz $

    Desc: PCI ids for ATI
    Lang: English
*/

#include <exec/types.h>
#include "ati.h"
#include "radeon.h"

struct ATIDevice {
    UWORD VendorID, ProductID __attribute__((packed));
    CardType Type;
    BOOL (*Init)(struct ati_staticdata*);
    BOOL masked_check;
};

extern const struct ATIDevice support[] __attribute__((section(".pci.supids")));

#endif /*IDS_H_*/
