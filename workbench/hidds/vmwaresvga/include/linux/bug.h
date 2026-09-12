/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BUG_H_
#define _LINUX_BUG_H_

#include <linux/compiler.h>
#include <linux/build_bug.h>
#include <linux/printk.h>

#define BUG()                   do { printk("BUG at %s:%d/%s()\n", __FILE__, __LINE__, __func__); } while (0)
#define BUG_ON(c)               do { if (unlikely(c)) BUG(); } while (0)

#define WARN(c, fmt, ...) ({                                                    \
    bool __ret_warn = !!(c);                                                    \
    if (unlikely(__ret_warn))                                                   \
        printk("WARNING at %s:%d/%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    unlikely(__ret_warn); })
#define WARN_ON(c) ({                                                           \
    bool __ret_warn = !!(c);                                                    \
    if (unlikely(__ret_warn))                                                   \
        printk("WARNING at %s:%d/%s(): %s\n", __FILE__, __LINE__, __func__, #c); \
    unlikely(__ret_warn); })
#define WARN_ONCE(c, fmt, ...) ({                                               \
    static bool __warned;                                                       \
    bool __ret_warn = !!(c);                                                    \
    if (unlikely(__ret_warn) && !__warned) {                                    \
        __warned = true;                                                        \
        printk("WARNING at %s:%d/%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    }                                                                           \
    unlikely(__ret_warn); })
#define WARN_ON_ONCE(c) ({                                                      \
    static bool __warned;                                                       \
    bool __ret_warn = !!(c);                                                    \
    if (unlikely(__ret_warn) && !__warned) {                                    \
        __warned = true;                                                        \
        printk("WARNING at %s:%d/%s(): %s\n", __FILE__, __LINE__, __func__, #c); \
    }                                                                           \
    unlikely(__ret_warn); })
#define WARN_TAINT(c, t, fmt, ...)      WARN(c, fmt, ##__VA_ARGS__)
#define WARN_TAINT_ONCE(c, t, fmt, ...) WARN_ONCE(c, fmt, ##__VA_ARGS__)
#define WARN_ON_SMP(c)                  WARN_ON(c)
#define __WARN()                        WARN_ON(1)
#define __WARN_printf(...)              printk(__VA_ARGS__)
#define BUG_ON_INVALID(c)               BUILD_BUG_ON_INVALID(c)
#define MAYBE_BUILD_BUG_ON(c)           (0)

#endif /* _LINUX_BUG_H_ */
