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

#endif /* _LINUX_EXPORT_H_ */
