/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_HWMON_SYSFS_H_
#define _LINUX_HWMON_SYSFS_H_

#include <linux/device.h>

struct sensor_device_attribute {
    struct device_attribute dev_attr;
    int index;
};

#define to_sensor_dev_attr(_dev_attr) container_of(_dev_attr, struct sensor_device_attribute, dev_attr)
#define SENSOR_DEVICE_ATTR(_name, _mode, _show, _store, _index) \
    struct sensor_device_attribute sensor_dev_attr_##_name = { .dev_attr = { .attr = { .name = #_name, .mode = _mode } }, .index = _index }

#endif
