/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MOD_DEVICETABLE_H_
#define _LINUX_MOD_DEVICETABLE_H_

#include <linux/types.h>
struct acpi_device_id { u8 id[16]; kernel_ulong_t driver_data; };
struct of_device_id { char name[32]; char type[32]; char compatible[128]; const void *data; };
struct platform_device_id { char name[20]; kernel_ulong_t driver_data; };
struct i2c_device_id { char name[20]; kernel_ulong_t driver_data; };
struct dmi_system_id;

#endif /* _LINUX_MOD_DEVICETABLE_H_ */
