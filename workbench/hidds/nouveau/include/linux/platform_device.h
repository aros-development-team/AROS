/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_PLATFORM_DEVICE_H_
#define _LINUX_PLATFORM_DEVICE_H_

#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
struct platform_device {
    const char *name;
    int id;
    struct device dev;
    u32 num_resources;
    struct resource *resource;
};
struct platform_driver {
    int (*probe)(struct platform_device *);
    void (*remove)(struct platform_device *);
    struct device_driver driver;
};
#define to_platform_device(x)   container_of((x), struct platform_device, dev)
#define platform_get_drvdata(p) dev_get_drvdata(&(p)->dev)
#define platform_set_drvdata(p, d) dev_set_drvdata(&(p)->dev, d)
#define platform_get_resource(p, t, n) ((struct resource *)NULL)
#define platform_get_irq(p, n)  (-ENXIO)
#define platform_driver_register(d) (0)
#define platform_driver_unregister(d) do { } while (0)
#define module_platform_driver(d)
#define platform_device_register_simple(...) ERR_PTR(-ENODEV)
#define platform_device_unregister(p) do { } while (0)
#define devm_platform_ioremap_resource(p, i) ERR_PTR(-ENODEV)

#endif /* _LINUX_PLATFORM_DEVICE_H_ */
