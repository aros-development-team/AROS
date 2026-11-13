/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_EVENTFD_H_
#define _LINUX_EVENTFD_H_

#include <linux/err.h>

struct eventfd_ctx;

static inline struct eventfd_ctx *eventfd_ctx_fdget(int fd) { return ERR_PTR(-EINVAL); }
static inline void eventfd_ctx_put(struct eventfd_ctx *ctx) { }
static inline void eventfd_signal(struct eventfd_ctx *ctx) { }

#endif
