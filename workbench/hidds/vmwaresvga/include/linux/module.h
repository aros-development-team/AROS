/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_MODULE_H_
#define _LINUX_MODULE_H_

#include <linux/export.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/elf.h>
#include <linux/bitmap.h>
#include <linux/string.h>
#include <linux/string_helpers.h>
#include <linux/list.h>
#include <linux/kobject.h>

struct module;
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define MODULE_VERSION(x)
#define MODULE_FIRMWARE(x)
#define MODULE_DEVICE_TABLE(a, b)
#define MODULE_ALIAS(x)
#define MODULE_IMPORT_NS(x)
#define MODULE_SOFTDEP(x)
#define MODULE_INFO(a, b)
#define MODULE_PARM_DESC(a, b)
#define module_param(a, b, c)
#define module_param_named(a, b, c, d)
#define module_param_unsafe(a, b, c)
#define module_param_named_unsafe(a, b, c, d)
#define try_module_get(m)       1
#define module_put(m)
#define module_name(m)          "nouveau"

#endif /* _LINUX_MODULE_H_ */
