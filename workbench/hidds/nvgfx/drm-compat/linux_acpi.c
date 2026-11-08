/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Linux's ACPI interface over acpica.library (CONFIG_ACPI builds only).
    The calls are ACPICA's; what differs is memory: ACPICA hands out
    AllocVec() buffers, Linux callers free them with kfree(), so every result
    is copied into one kmalloc() block.
*/

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <linux/acpi.h>

#include "linux_acpi_bridge.h"

/* Usable only if ACPICA's objects have the layout of union acpi_object */
bool compat_acpi_available(void)
{
    static int checked;

    if (!acpi_bridge_open())
        return false;
    if (!checked) {
        checked = 1;
        if (acpi_bridge_object_size() != sizeof(union acpi_object)) {
            pr_err("acpi: ACPICA objects are %lu bytes, %lu expected: ACPI not used\n",
                   acpi_bridge_object_size(), (unsigned long)sizeof(union acpi_object));
            checked = -1;
        }
    }
    return checked > 0;
}

static size_t object_extra(const union acpi_object *obj)
{
    size_t n = 0;
    u32 i;

    switch (obj->type) {
    case ACPI_TYPE_STRING:
        return obj->string.length + 1;
    case ACPI_TYPE_BUFFER:
        return obj->buffer.length;
    case ACPI_TYPE_PACKAGE:
        /* room to align the element array (object_copy()) */
        n = 7 + obj->package.count * sizeof(union acpi_object);
        for (i = 0; i < obj->package.count; i++)
            n += object_extra(&obj->package.elements[i]);
        return n;
    }
    return 0;
}

/* copies src into dst, its strings, buffers and elements to *pool */
static void object_copy(union acpi_object *dst, const union acpi_object *src, u8 **pool)
{
    u32 i;

    *dst = *src;
    switch (src->type) {
    case ACPI_TYPE_STRING:
        dst->string.pointer = (char *)*pool;
        memcpy(*pool, src->string.pointer, src->string.length);
        (*pool)[src->string.length] = '\0';
        *pool += src->string.length + 1;
        break;
    case ACPI_TYPE_BUFFER:
        dst->buffer.pointer = *pool;
        memcpy(*pool, src->buffer.pointer, src->buffer.length);
        *pool += src->buffer.length;
        break;
    case ACPI_TYPE_PACKAGE:
        *pool = (u8 *)(((uintptr_t)*pool + 7) & ~(uintptr_t)7);
        dst->package.elements = (union acpi_object *)*pool;
        *pool += src->package.count * sizeof(union acpi_object);
        for (i = 0; i < src->package.count; i++)
            object_copy(&dst->package.elements[i], &src->package.elements[i], pool);
        break;
    }
}

/* the tree at obj as one kmalloc() block */
static union acpi_object *object_dup(const union acpi_object *obj, size_t *size)
{
    union acpi_object *copy;
    u8 *pool;

    *size = sizeof(*copy) + object_extra(obj);
    if (!(copy = kmalloc(*size, GFP_KERNEL)))
        return NULL;
    pool = (u8 *)(copy + 1);
    object_copy(copy, obj, &pool);
    return copy;
}

acpi_status acpi_evaluate_object(acpi_handle handle, acpi_string pathname,
                                 struct acpi_object_list *args, struct acpi_buffer *ret)
{
    void *result = NULL;
    union acpi_object *copy;
    acpi_status status;
    size_t size;

    if (!compat_acpi_available())
        return AE_NOT_FOUND;
    status = acpi_bridge_evaluate(handle, pathname, args ? args->count : 0,
                                  args ? args->pointer : NULL, &result);
    if (ACPI_FAILURE(status) || !ret) {
        if (result)
            acpi_bridge_free(result);
        return status;
    }
    if (!result) {
        ret->length = 0;
        return status;
    }
    copy = object_dup(result, &size);
    acpi_bridge_free(result);
    if (!copy)
        return AE_NO_MEMORY;
    if (ret->length == ACPI_ALLOCATE_BUFFER) {
        ret->pointer = copy;
        ret->length = size;
        return AE_OK;
    }
    /* the caller's own buffer: the tree moves, so copy it again in place */
    if (ret->length < size) {
        ret->length = size;
        kfree(copy);
        return AE_BUFFER_OVERFLOW;
    }
    {
        u8 *pool = (u8 *)ret->pointer + sizeof(union acpi_object);

        object_copy(ret->pointer, copy, &pool);
    }
    ret->length = size;
    kfree(copy);
    return AE_OK;
}

acpi_status acpi_evaluate_integer(acpi_handle handle, acpi_string pathname,
                                  struct acpi_object_list *args, unsigned long long *data)
{
    struct acpi_buffer out = { ACPI_ALLOCATE_BUFFER, NULL };
    union acpi_object *obj;
    acpi_status status;

    status = acpi_evaluate_object(handle, pathname, args, &out);
    if (ACPI_FAILURE(status))
        return status;
    obj = out.pointer;
    if (!obj || obj->type != ACPI_TYPE_INTEGER) {
        kfree(obj);
        return AE_BAD_DATA;
    }
    *data = obj->integer.value;
    kfree(obj);
    return AE_OK;
}

/* As Linux's drivers/acpi/utils.c */
union acpi_object *acpi_evaluate_dsm(acpi_handle handle, const guid_t *guid, u64 rev,
                                     u64 func, union acpi_object *argv4)
{
    struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER, NULL };
    union acpi_object params[4];
    struct acpi_object_list input = { 4, params };
    acpi_status ret;

    memset(params, 0, sizeof(params));
    params[0].type = ACPI_TYPE_BUFFER;
    params[0].buffer.length = 16;
    params[0].buffer.pointer = (u8 *)guid;
    params[1].type = ACPI_TYPE_INTEGER;
    params[1].integer.value = rev;
    params[2].type = ACPI_TYPE_INTEGER;
    params[2].integer.value = func;
    if (argv4) {
        params[3] = *argv4;
    } else {
        params[3].type = ACPI_TYPE_PACKAGE;
        params[3].package.count = 0;
        params[3].package.elements = NULL;
    }

    ret = acpi_evaluate_object(handle, "_DSM", &input, &buf);
    if (ACPI_SUCCESS(ret))
        return (union acpi_object *)buf.pointer;
    if (ret != AE_NOT_FOUND)
        acpi_handle_warn(handle, "failed to evaluate _DSM rev:%lld func:%lld (0x%x)\n",
                         (long long)rev, (long long)func, ret);
    return NULL;
}

bool acpi_check_dsm(acpi_handle handle, const guid_t *guid, u64 rev, u64 funcs)
{
    union acpi_object *obj;
    u64 mask = 0;
    u32 i;

    if (funcs == 0)
        return false;
    obj = acpi_evaluate_dsm(handle, guid, rev, 0, NULL);
    if (!obj)
        return false;
    /* For compatibility, old BIOSes may return an integer */
    if (obj->type == ACPI_TYPE_INTEGER)
        mask = obj->integer.value;
    else if (obj->type == ACPI_TYPE_BUFFER)
        for (i = 0; i < obj->buffer.length && i < 8; i++)
            mask |= (((u64)obj->buffer.pointer[i]) << (i * 8));
    ACPI_FREE(obj);
    return (mask & 0x1) && (mask & funcs) == funcs;
}

acpi_status acpi_get_handle(acpi_handle parent, const char *pathname, acpi_handle *ret)
{
    if (!compat_acpi_available())
        return AE_NOT_FOUND;
    return acpi_bridge_get_handle(parent, pathname, ret);
}

bool acpi_has_method(acpi_handle handle, char *name)
{
    acpi_handle tmp;

    return ACPI_SUCCESS(acpi_get_handle(handle, name, &tmp));
}

acpi_status acpi_get_name(acpi_handle handle, u32 name_type, struct acpi_buffer *ret)
{
    char *name = NULL, *copy;
    acpi_status status;
    size_t len;

    if (!compat_acpi_available())
        return AE_NOT_FOUND;
    status = acpi_bridge_get_name(handle, name_type, &name);
    if (ACPI_FAILURE(status))
        return status;
    len = strlen(name) + 1;
    if (ret->length == ACPI_ALLOCATE_BUFFER) {
        copy = kmalloc(len, GFP_KERNEL);
        if (copy)
            memcpy(copy, name, len);
        acpi_bridge_free(name);
        if (!copy)
            return AE_NO_MEMORY;
        ret->pointer = copy;
        ret->length = len;
        return AE_OK;
    }
    if (ret->length < len) {
        acpi_bridge_free(name);
        ret->length = len;
        return AE_BUFFER_OVERFLOW;
    }
    memcpy(ret->pointer, name, len);
    ret->length = len;
    acpi_bridge_free(name);
    return AE_OK;
}

acpi_status acpi_get_next_object(acpi_object_type type, acpi_handle parent,
                                 acpi_handle child, acpi_handle *next)
{
    if (!compat_acpi_available())
        return AE_NOT_FOUND;
    return acpi_bridge_get_next_object(type, parent, child, next);
}

const char *acpi_format_exception(acpi_status status)
{
    return acpi_bridge_format_exception(status);
}

void acpi_handle_printk(const char *level, acpi_handle handle, const char *fmt, ...)
{
    struct acpi_buffer path = { ACPI_ALLOCATE_BUFFER, NULL };
    char msg[192];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    if (ACPI_SUCCESS(acpi_get_name(handle, ACPI_FULL_PATHNAME, &path))) {
        printk("%sACPI: %s: %s", level, (char *)path.pointer, msg);
        kfree(path.pointer);
    } else {
        printk("%sACPI: %s", level, msg);
    }
}

static unsigned char pci_cfg8(unsigned int bus, unsigned int devfn, int where)
{
    struct pci_bus b;
    u8 val = 0xff;

    memset(&b, 0, sizeof(b));
    b.number = bus;
    if (pci_bus_read_config_byte(&b, devfn, where, &val))
        return 0xff;
    return val;
}

/* The ACPI device node of a PCI device, looked up once per device */
acpi_handle compat_acpi_handle(struct device *dev)
{
    static struct pci_dev *known;
    static acpi_handle handle;
    struct pci_dev *pdev;

    if (!dev || !dev->is_pci || !compat_acpi_available())
        return NULL;
    pdev = to_pci_dev(dev);
    if (pdev != known) {
        known = pdev;
        handle = acpi_bridge_find_pci(pdev->bus->number, pdev->devfn, pci_cfg8);
    }
    return handle;
}
