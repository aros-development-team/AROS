#ifndef HIDD_VIRTIO_H
#define HIDD_VIRTIO_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: VirtIO bus driver HIDD definitions
    Lang: english
*/

#define CLID_Hidd_VirtIO        "hidd.virtio"
#define IID_Hidd_VirtIO         CLID_Hidd_VirtIO

#include <interface/Hidd_VirtIOBus.h>
#define CLID_Hidd_VirtIOBus     IID_Hidd_VirtIOBus
#include <interface/Hidd_VirtIOUnit.h>
#define CLID_Hidd_VirtIOUnit    IID_Hidd_VirtIOUnit

#endif