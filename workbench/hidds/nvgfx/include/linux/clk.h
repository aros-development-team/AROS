#include <linux/err.h>
#include <linux/device.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CLK_H_
#define _LINUX_CLK_H_

struct clk;
static inline struct clk *clk_get(struct device *dev, const char *id) { return ERR_PTR(-ENODEV); }
static inline struct clk *devm_clk_get(struct device *dev, const char *id) { return ERR_PTR(-ENODEV); }
static inline void clk_put(struct clk *c) { }
static inline int clk_prepare_enable(struct clk *c) { return -ENODEV; }
static inline void clk_disable_unprepare(struct clk *c) { }
static inline unsigned long clk_get_rate(struct clk *c) { return 0; }
static inline int clk_set_rate(struct clk *c, unsigned long r) { return -ENODEV; }
static inline int clk_enable(struct clk *c) { return -ENODEV; }
static inline void clk_disable(struct clk *c) { }
static inline int clk_prepare(struct clk *c) { return -ENODEV; }
static inline void clk_unprepare(struct clk *c) { }

#endif /* _LINUX_CLK_H_ */
