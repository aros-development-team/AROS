/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_PANIC_H_
#define _LINUX_PANIC_H_

#include <linux/printk.h>

#define panic(fmt, ...)         do { printk("PANIC: " fmt, ##__VA_ARGS__); for (;;) { } } while (0)
#define panic_on_warn           0

#endif /* _LINUX_PANIC_H_ */
