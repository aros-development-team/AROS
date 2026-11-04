#include <linux/stddef.h>
#include <linux/types.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_COMPONENT_H_
#define _LINUX_COMPONENT_H_

struct component_ops { int (*bind)(struct device *, struct device *, void *); void (*unbind)(struct device *, struct device *, void *); };
static inline int component_add(struct device *dev, const struct component_ops *ops) { return -ENODEV; }
static inline void component_del(struct device *dev, const struct component_ops *ops) { }

#endif /* _LINUX_COMPONENT_H_ */
