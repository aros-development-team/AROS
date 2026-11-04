/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_ACPI_H_
#define _LINUX_ACPI_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/uuid.h>

/* no ACPI on this platform: every query answers "absent" */
typedef void *acpi_handle;
typedef u32 acpi_status;
typedef u64 acpi_size;
typedef char *acpi_string;
typedef u32 acpi_object_type;
#define AE_OK                   0
#define AE_ERROR                1
#define AE_NOT_FOUND            5
#define AE_NOT_EXIST            0x1004
#define AE_BAD_PARAMETER        0x1001
#define ACPI_FAILURE(s)         ((s) != AE_OK)
#define ACPI_SUCCESS(s)         ((s) == AE_OK)
#define ACPI_HANDLE(dev)        ((acpi_handle)NULL)
#define ACPI_COMPANION(dev)     ((struct acpi_device *)NULL)
#define ACPI_ALLOCATE_BUFFER    ((acpi_size)-1)
#define ACPI_TYPE_INTEGER       0x01
#define ACPI_TYPE_STRING        0x02
#define ACPI_TYPE_BUFFER        0x03
#define ACPI_TYPE_PACKAGE       0x04
#define ACPI_TYPE_ANY           0x00
#define ACPI_TYPE_LOCAL_REFERENCE 0x14
#define ACPI_ROOT_OBJECT        ((acpi_handle)NULL)
#define ACPI_UINT32_MAX         0xffffffff
#define ACPI_ADR_SPACE_SYSTEM_MEMORY 0
#define ACPI_ADR_SPACE_SYSTEM_IO 1
#define ACPI_VIDEO_NOTIFY_PROBE 0x81
#define ACPI_VIDEO_DEVICE_POST  0
#define ACPI_VIDEO_CLASS        "video"
struct acpi_buffer { acpi_size length; void *pointer; };
struct acpi_object_list { u32 count; union acpi_object *pointer; };
union acpi_object {
    acpi_object_type type;
    struct { acpi_object_type type; u64 value; } integer;
    struct { acpi_object_type type; u32 length; char *pointer; } string;
    struct { acpi_object_type type; u32 length; u8 *pointer; } buffer;
    struct { acpi_object_type type; u32 count; union acpi_object *elements; } package;
    struct { acpi_object_type type; acpi_object_type actual_type; acpi_handle handle; } reference;
};
struct acpi_device { struct device dev; acpi_handle handle; };
struct acpi_bus_event { char device_class[20]; char bus_id[15]; u32 type; u32 data; };
struct acpi_table_header;
struct notifier_block;
typedef u32 (*acpi_osd_handler)(void *context);
typedef void (*acpi_notify_handler)(acpi_handle handle, u32 event, void *data);
static inline bool has_acpi_companion(struct device *dev) { return false; }
static inline bool acpi_has_method(acpi_handle handle, char *name) { return false; }
static inline acpi_status acpi_evaluate_object(acpi_handle o, acpi_string p, struct acpi_object_list *a, struct acpi_buffer *r) { return AE_NOT_FOUND; }
static inline acpi_status acpi_evaluate_integer(acpi_handle h, acpi_string p, struct acpi_object_list *a, unsigned long long *d) { return AE_NOT_FOUND; }
static inline union acpi_object *acpi_evaluate_dsm(acpi_handle h, const void *g, u64 rev, u64 func, union acpi_object *argv4) { return NULL; }
static inline union acpi_object *acpi_evaluate_dsm_typed(acpi_handle h, const void *g, u64 rev, u64 func, union acpi_object *argv4, acpi_object_type type) { return NULL; }
static inline bool acpi_check_dsm(acpi_handle h, const void *g, u64 rev, u64 funcs) { return false; }
static inline acpi_status acpi_get_handle(acpi_handle p, const char *pathname, acpi_handle *ret) { return AE_NOT_FOUND; }
static inline acpi_status acpi_get_name(acpi_handle h, u32 t, struct acpi_buffer *b) { return AE_NOT_FOUND; }
static inline acpi_status acpi_get_next_object(acpi_object_type t, acpi_handle p, acpi_handle c, acpi_handle *r) { return AE_NOT_FOUND; }
static inline acpi_status acpi_get_table(char *sig, u32 inst, struct acpi_table_header **t) { return AE_NOT_FOUND; }
static inline void acpi_put_table(struct acpi_table_header *t) { }
static inline int register_acpi_notifier(struct notifier_block *nb) { return 0; }
static inline int unregister_acpi_notifier(struct notifier_block *nb) { return 0; }
static inline bool acpi_video_backlight_use_native(void) { return true; }
static inline int acpi_video_register(void) { return 0; }
static inline void acpi_video_unregister(void) { }
static inline int acpi_video_get_edid(struct acpi_device *d, int type, int id, void **edid) { return -ENODEV; }
static inline int acpi_target_system_state(void) { return 0; }
static inline bool acpi_dev_present(const char *hid, const char *uid, s64 hrv) { return false; }
static inline const char *acpi_dev_name(struct acpi_device *adev) { return NULL; }
static inline void acpi_handle_info(acpi_handle h, const char *fmt, ...) { }
static inline void acpi_handle_debug(acpi_handle h, const char *fmt, ...) { }
static inline void acpi_handle_err(acpi_handle h, const char *fmt, ...) { }
static inline void acpi_handle_warn(acpi_handle h, const char *fmt, ...) { }
#define acpi_disabled                   (1)
#define acpi_lid_open()                 (1)
#define ACPI_STATE_S3                   3
#define ACPI_STATE_S4                   4
#define ACPI_HANDLE_FWNODE(f)           NULL
#define ACPI_DEVICE_CLASS(c, m)         .class = (c), .class_mask = (m)
#define ACPI_STA_DEVICE_PRESENT         0x01
#define ACPI_STA_DEVICE_ENABLED         0x02
#define ACPI_STA_DEVICE_FUNCTIONING     0x08
struct acpi_video_device_attrib;

#endif /* _LINUX_ACPI_H_ */
