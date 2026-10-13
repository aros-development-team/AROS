/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_MFD_CORE_H_
#define _LINUX_MFD_CORE_H_

#include <linux/types.h>

struct resource;
struct device;

struct mfd_cell {
    const char *name;
    int id;
    int num_resources;
    const struct resource *resources;
    void *platform_data;
    size_t pdata_size;
};

static inline int mfd_add_hotplug_devices(struct device *parent, const struct mfd_cell *cells, int n_devs) { return -ENODEV; }
static inline void mfd_remove_devices(struct device *parent) { }

#endif
