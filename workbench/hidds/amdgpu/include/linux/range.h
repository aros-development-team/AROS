/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_RANGE_H_
#define _LINUX_RANGE_H_
#include <linux/types.h>

struct range {
    u64 start;
    u64 end;
};

static inline u64 range_len(const struct range *range)
{
    return range->end - range->start + 1;
}

#endif
