/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DMI_H_
#define _LINUX_DMI_H_

#include <linux/mod_devicetable.h>
#include <linux/list.h>
#include <linux/kobject.h>
struct dmi_strmatch { unsigned char slot:7; unsigned char exact_match:1; char substr[79]; };
struct dmi_system_id { int (*callback)(const struct dmi_system_id *); const char *ident; struct dmi_strmatch matches[4]; void *driver_data; };
#define DMI_MATCH(a, b)         { .slot = (a), .substr = b }
#define DMI_EXACT_MATCH(a, b)   { .slot = (a), .exact_match = 1, .substr = b }
enum dmi_field { DMI_NONE, DMI_BIOS_VENDOR, DMI_BIOS_VERSION, DMI_BIOS_DATE, DMI_BIOS_RELEASE, DMI_EC_FIRMWARE_RELEASE, DMI_SYS_VENDOR, DMI_PRODUCT_NAME, DMI_PRODUCT_VERSION, DMI_PRODUCT_SERIAL, DMI_PRODUCT_UUID, DMI_PRODUCT_SKU, DMI_PRODUCT_FAMILY, DMI_BOARD_VENDOR, DMI_BOARD_NAME, DMI_BOARD_VERSION, DMI_BOARD_SERIAL, DMI_BOARD_ASSET_TAG, DMI_CHASSIS_VENDOR, DMI_CHASSIS_TYPE, DMI_CHASSIS_VERSION, DMI_CHASSIS_SERIAL, DMI_CHASSIS_ASSET_TAG, DMI_STRING_MAX, DMI_OEM_STRING };
static inline int dmi_check_system(const struct dmi_system_id *list) { return 0; }
static inline const struct dmi_system_id *dmi_first_match(const struct dmi_system_id *list) { return NULL; }
static inline const char *dmi_get_system_info(int field) { return NULL; }
static inline bool dmi_match(enum dmi_field f, const char *str) { return false; }
static inline int dmi_name_in_vendors(const char *str) { return 0; }

#endif /* _LINUX_DMI_H_ */
