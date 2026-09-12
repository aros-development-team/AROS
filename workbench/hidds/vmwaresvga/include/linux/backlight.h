/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_BACKLIGHT_H_
#define _LINUX_BACKLIGHT_H_

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/fb.h>
struct backlight_device;
struct backlight_properties { int brightness; int max_brightness; int power; int type; unsigned int state; };
struct backlight_ops { int (*update_status)(struct backlight_device *); int (*get_brightness)(struct backlight_device *); };
struct backlight_device { struct backlight_properties props; struct device dev; const struct backlight_ops *ops; };
enum backlight_type { BACKLIGHT_RAW = 1, BACKLIGHT_PLATFORM, BACKLIGHT_FIRMWARE };
#define BL_CORE_SUSPENDED       (1 << 0)
#define BL_CORE_FBBLANK         (1 << 1)
#define backlight_get_brightness(bd) ((bd)->props.brightness)
#define backlight_is_blank(bd)  (0)
static inline void backlight_device_unregister(struct backlight_device *bd) { }
static inline struct backlight_device *backlight_device_register(const char *n, struct device *d, void *p, const struct backlight_ops *o, const struct backlight_properties *pr) { return ERR_PTR(-ENODEV); }
static inline void *bl_get_data(struct backlight_device *bd) { return NULL; }
static inline void backlight_update_status(struct backlight_device *bd) { }

#endif /* _LINUX_BACKLIGHT_H_ */
