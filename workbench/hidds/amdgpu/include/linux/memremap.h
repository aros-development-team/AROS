/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_MEMREMAP_H_
#define _LINUX_MEMREMAP_H_
#include <linux/range.h>

enum memory_type {
    MEMORY_DEVICE_PRIVATE = 1,
    MEMORY_DEVICE_COHERENT,
    MEMORY_DEVICE_FS_DAX,
    MEMORY_DEVICE_GENERIC,
    MEMORY_DEVICE_PCI_P2PDMA,
};

struct dev_pagemap_ops;

struct dev_pagemap {
    struct range range;
    enum memory_type type;
    unsigned int flags;
    unsigned long vmemmap_shift;
    const struct dev_pagemap_ops *ops;
    void *owner;
    int nr_range;
};

#endif
