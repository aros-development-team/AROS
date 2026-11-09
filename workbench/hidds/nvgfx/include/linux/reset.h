#include <linux/err.h>
#include <linux/device.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_RESET_H_
#define _LINUX_RESET_H_

struct reset_control;
static inline struct reset_control *reset_control_get(struct device *dev, const char *id) { return ERR_PTR(-ENODEV); }
static inline void reset_control_put(struct reset_control *r) { }
static inline int reset_control_assert(struct reset_control *r) { return -ENODEV; }
static inline int reset_control_deassert(struct reset_control *r) { return -ENODEV; }
#define devm_reset_control_get(d, i) reset_control_get(d, i)

#endif /* _LINUX_RESET_H_ */
