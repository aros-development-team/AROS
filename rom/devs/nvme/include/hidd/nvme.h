#ifndef HIDD_NVME_H
#define HIDD_NVME_H

/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NVME bus driver HIDD definitions
    Lang: english
*/

#define CLID_Hidd_NVME          "hidd.nvme"
#define IID_Hidd_NVME           CLID_Hidd_NVME

#include <interface/Hidd_NVMEBus.h>
#define CLID_Hidd_NVMEBus       IID_Hidd_NVMEBus
#include <interface/Hidd_NVMEUnit.h>
#define CLID_Hidd_NVMEUnit       IID_Hidd_NVMEUnit

#endif
