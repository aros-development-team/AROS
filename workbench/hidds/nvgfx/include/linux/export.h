/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_EXPORT_H_
#define _LINUX_EXPORT_H_

#define EXPORT_SYMBOL(x)
#define EXPORT_SYMBOL_GPL(x)
#define EXPORT_SYMBOL_NS(x, ns)
#define EXPORT_SYMBOL_NS_GPL(x, ns)
#define EXPORT_SYMBOL_FOR_MODULES(x, m)
#define THIS_MODULE             ((struct module *)0)

/* module metadata, for sources that take it from here rather than module.h */
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define MODULE_VERSION(x)
#include <linux/moduleparam.h>

#endif /* _LINUX_EXPORT_H_ */
