/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_RATELIMIT_H_
#define _LINUX_RATELIMIT_H_

#include <linux/ratelimit_types.h>

#define ratelimit_state_init(rs, i, b)      do { (rs)->interval = (i); (rs)->burst = (b); } while (0)
#define ratelimit_set_flags(rs, f)          do { (rs)->flags = (f); } while (0)
#define ratelimit_state_reset_interval(rs, i) do { (rs)->interval = (i); } while (0)

#endif
