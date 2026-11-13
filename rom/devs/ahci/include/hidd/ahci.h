#ifndef HIDD_AHCI_H
#define HIDD_AHCI_H

/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AHCI bus driver HIDD definitions
    Lang: english
*/

#define CLID_Hidd_AHCI          "hidd.ahci"
#define IID_Hidd_AHCI           CLID_Hidd_AHCI

#include <interface/Hidd_AHCIBus.h>
#define CLID_Hidd_AHCIBus       IID_Hidd_AHCIBus
#include <interface/Hidd_AHCIUnit.h>
#define CLID_Hidd_AHCIUnit       IID_Hidd_AHCIUnit

#endif
