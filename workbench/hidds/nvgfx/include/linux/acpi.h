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

/*
 * Linux's ACPI interface is ACPICA's own: the types, status codes and object
 * layout below are ACPICA's. With CONFIG_ACPI (the NVGFX_ACPI build option)
 * the calls go to acpica.library (drm-compat/linux_acpi.c); without it, or
 * on a machine without ACPI, every query answers "absent".
 */
typedef void *acpi_handle;
typedef u32 acpi_status;
typedef u64 acpi_size;
typedef char *acpi_string;
typedef u32 acpi_object_type;
#define AE_OK                   0x0000
#define AE_ERROR                0x0001
#define AE_NO_MEMORY            0x0004
#define AE_NOT_FOUND            0x0005
#define AE_NOT_EXIST            0x0006
#define AE_TYPE                 0x0008
#define AE_BUFFER_OVERFLOW      0x000B
#define AE_BAD_PARAMETER        0x1001
#define AE_BAD_DATA             0x2003
#define ACPI_FAILURE(s)         ((s) != AE_OK)
#define ACPI_SUCCESS(s)         ((s) == AE_OK)
#define ACPI_ALLOCATE_BUFFER    ((acpi_size)-1)
#define ACPI_FULL_PATHNAME      0
#define ACPI_SINGLE_NAME        1
#define ACPI_TYPE_ANY           0x00
#define ACPI_TYPE_INTEGER       0x01
#define ACPI_TYPE_STRING        0x02
#define ACPI_TYPE_BUFFER        0x03
#define ACPI_TYPE_PACKAGE       0x04
#define ACPI_TYPE_DEVICE        0x06
#define ACPI_TYPE_LOCAL_REFERENCE 0x14
#define ACPI_ROOT_OBJECT        ((acpi_handle)NULL)
#define ACPI_UINT32_MAX         0xffffffff
#define ACPI_ADR_SPACE_SYSTEM_MEMORY 0
#define ACPI_ADR_SPACE_SYSTEM_IO 1
#define ACPI_VIDEO_NOTIFY_PROBE 0x81
#define ACPI_VIDEO_DEVICE_POST  0
#define ACPI_VIDEO_DISPLAY_LCD  4
#define ACPI_VIDEO_CLASS        "video"
struct acpi_buffer { acpi_size length; void *pointer; };
struct acpi_object_list { u32 count; union acpi_object *pointer; };
/* ACPICA's ACPI_OBJECT, every member, so arrays of them line up */
union acpi_object {
    acpi_object_type type;
    struct { acpi_object_type type; u64 value; } integer;
    struct { acpi_object_type type; u32 length; char *pointer; } string;
    struct { acpi_object_type type; u32 length; u8 *pointer; } buffer;
    struct { acpi_object_type type; u32 count; union acpi_object *elements; } package;
    struct { acpi_object_type type; acpi_object_type actual_type; acpi_handle handle; } reference;
    struct { acpi_object_type type; u32 proc_id; u64 pblk_address; u32 pblk_length; } processor;
    struct { acpi_object_type type; u32 system_level; u32 resource_order; } power_resource;
};
struct acpi_device { struct device dev; acpi_handle handle; };
struct acpi_bus_event { char device_class[20]; char bus_id[15]; u32 type; u32 data; };
struct acpi_table_header;
struct notifier_block;
typedef u32 (*acpi_osd_handler)(void *context);
typedef void (*acpi_notify_handler)(acpi_handle handle, u32 event, void *data);

/* Results allocated for the caller (ACPI_ALLOCATE_BUFFER) are kmalloc()ed. */
#define ACPI_FREE(p)            kfree(p)

#if defined(CONFIG_ACPI)

bool compat_acpi_available(void);
acpi_handle compat_acpi_handle(struct device *dev);
#define acpi_disabled           (!compat_acpi_available())
#define ACPI_HANDLE(dev)        compat_acpi_handle(dev)
#define has_acpi_companion(dev) (compat_acpi_handle(dev) != NULL)

bool acpi_has_method(acpi_handle handle, char *name);
acpi_status acpi_evaluate_object(acpi_handle handle, acpi_string pathname,
                                 struct acpi_object_list *args, struct acpi_buffer *ret);
acpi_status acpi_evaluate_integer(acpi_handle handle, acpi_string pathname,
                                  struct acpi_object_list *args, unsigned long long *data);
union acpi_object *acpi_evaluate_dsm(acpi_handle handle, const guid_t *guid, u64 rev,
                                     u64 func, union acpi_object *argv4);
bool acpi_check_dsm(acpi_handle handle, const guid_t *guid, u64 rev, u64 funcs);
acpi_status acpi_get_handle(acpi_handle parent, const char *pathname, acpi_handle *ret);
acpi_status acpi_get_name(acpi_handle handle, u32 name_type, struct acpi_buffer *ret);
acpi_status acpi_get_next_object(acpi_object_type type, acpi_handle parent,
                                 acpi_handle child, acpi_handle *next);
const char *acpi_format_exception(acpi_status status);
void acpi_handle_printk(const char *level, acpi_handle handle, const char *fmt, ...);
#define acpi_handle_err(h, fmt, ...)   acpi_handle_printk(KERN_ERR, h, fmt, ##__VA_ARGS__)
#define acpi_handle_warn(h, fmt, ...)  acpi_handle_printk(KERN_WARNING, h, fmt, ##__VA_ARGS__)
#define acpi_handle_info(h, fmt, ...)  acpi_handle_printk(KERN_INFO, h, fmt, ##__VA_ARGS__)
#define acpi_handle_debug(h, fmt, ...) do { } while (0)

static inline union acpi_object *acpi_evaluate_dsm_typed(acpi_handle handle, const guid_t *guid,
        u64 rev, u64 func, union acpi_object *argv4, acpi_object_type type)
{
    union acpi_object *obj = acpi_evaluate_dsm(handle, guid, rev, func, argv4);

    if (obj && obj->type != type) {
        ACPI_FREE(obj);
        obj = NULL;
    }
    return obj;
}

#else /* !CONFIG_ACPI */

#define acpi_disabled                   (1)
#define ACPI_HANDLE(dev)                ((acpi_handle)NULL)
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
static inline const char *acpi_format_exception(acpi_status status) { return "AE_NOT_FOUND"; }
static inline void acpi_handle_info(acpi_handle h, const char *fmt, ...) { }
static inline void acpi_handle_debug(acpi_handle h, const char *fmt, ...) { }
static inline void acpi_handle_err(acpi_handle h, const char *fmt, ...) { }
static inline void acpi_handle_warn(acpi_handle h, const char *fmt, ...) { }

#endif /* CONFIG_ACPI */

/* Not provided either way: no ACPI device objects, video bus, lid or tables */
#define ACPI_COMPANION(dev)     ((struct acpi_device *)NULL)
static inline acpi_status acpi_get_table(char *sig, u32 inst, struct acpi_table_header **t) { return AE_NOT_FOUND; }
static inline void acpi_put_table(struct acpi_table_header *t) { }
static inline int register_acpi_notifier(struct notifier_block *nb) { return 0; }
static inline int unregister_acpi_notifier(struct notifier_block *nb) { return 0; }
static inline bool acpi_video_backlight_use_native(void) { return true; }
static inline void acpi_video_register_backlight(void) { }
static inline int acpi_video_register(void) { return 0; }
static inline void acpi_video_unregister(void) { }
static inline int acpi_video_get_edid(struct acpi_device *d, int type, int id, void **edid) { return -ENODEV; }
static inline int acpi_target_system_state(void) { return 0; }
static inline bool acpi_dev_present(const char *hid, const char *uid, s64 hrv) { return false; }
static inline const char *acpi_dev_name(struct acpi_device *adev) { return NULL; }
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
