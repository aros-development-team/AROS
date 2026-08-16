/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ASM_GENERIC_VIDEO_H_
#define _ASM_GENERIC_VIDEO_H_
#include <linux/types.h>
struct device;
static inline bool video_is_primary_device(struct device *dev) { return false; }
#endif
