/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ASM_GENERIC_VIDEO_H_
#define _ASM_GENERIC_VIDEO_H_
#include <linux/types.h>
struct device;
/*
 * The card that carried the firmware console: the hidd records it when
 * it takes a boot display over from graphics.library.
 */
extern bool nouveau_aros_boot_display;
static inline bool video_is_primary_device(struct device *dev) { return nouveau_aros_boot_display; }
#endif
