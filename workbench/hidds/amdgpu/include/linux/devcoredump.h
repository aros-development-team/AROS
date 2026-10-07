/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_DEVCOREDUMP_H_
#define _LINUX_DEVCOREDUMP_H_

#include <linux/types.h>

struct device;

static inline void dev_coredumpm(struct device *dev, void *owner, void *data, size_t datalen, gfp_t gfp,
    ssize_t (*read)(char *buffer, loff_t offset, size_t count, void *data, size_t datalen),
    void (*free)(void *data)) { if (free) free(data); }

#endif
