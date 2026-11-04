#include <linux/err.h>
#include <linux/device.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_REGULATOR_CONSUMER_H_
#define _LINUX_REGULATOR_CONSUMER_H_

struct regulator;
static inline struct regulator *regulator_get(struct device *dev, const char *id) { return ERR_PTR(-ENODEV); }
static inline void regulator_put(struct regulator *r) { }
static inline int regulator_enable(struct regulator *r) { return -ENODEV; }
static inline int regulator_disable(struct regulator *r) { return -ENODEV; }
static inline int regulator_get_voltage(struct regulator *r) { return -ENODEV; }
static inline int regulator_set_voltage(struct regulator *r, int min, int max) { return -ENODEV; }
#define devm_regulator_get(d, i) regulator_get(d, i)

#endif /* _LINUX_REGULATOR_CONSUMER_H_ */
