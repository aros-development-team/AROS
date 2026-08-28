#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_LEDS_H_
#define _LINUX_LEDS_H_

struct led_classdev { const char *name; int brightness; int max_brightness; };
enum led_brightness { LED_OFF = 0, LED_ON = 1, LED_HALF = 127, LED_FULL = 255 };
static inline int led_classdev_register(struct device *d, struct led_classdev *l) { return -ENODEV; }
static inline void led_classdev_unregister(struct led_classdev *l) { }

#endif /* _LINUX_LEDS_H_ */
