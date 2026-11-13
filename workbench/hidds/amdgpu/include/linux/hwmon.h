/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_HWMON_H_
#define _LINUX_HWMON_H_

#include <linux/types.h>

struct device;
struct attribute_group;

static inline struct device *hwmon_device_register_with_groups(struct device *dev, const char *name, void *drvdata, const struct attribute_group **groups) { return NULL; }
static inline void hwmon_device_unregister(struct device *dev) { }

#endif
