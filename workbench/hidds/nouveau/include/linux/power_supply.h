#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/leds.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_POWER_SUPPLY_H_
#define _LINUX_POWER_SUPPLY_H_

static inline int power_supply_is_system_supplied(void) { return -ENOSYS; }

#endif /* _LINUX_POWER_SUPPLY_H_ */
