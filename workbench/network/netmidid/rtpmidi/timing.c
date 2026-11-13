/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "rtpmidi/timing.h"

#include <limits.h>

int rtpmidi_clock_due(uint32_t timestamp, uint64_t local_now,
                 int64_t peer_to_local, uint64_t *local_due)
{
    uint64_t peer_now;
    uint32_t difference;
    int64_t delta;
    if (!local_due)
        return -1;
    if (peer_to_local >= 0) {
        if (local_now < (uint64_t)peer_to_local)
            return -1;
        peer_now = local_now - (uint64_t)peer_to_local;
    } else {
        uint64_t magnitude = (uint64_t)(-(peer_to_local + 1)) + 1;
        if (local_now > UINT64_MAX - magnitude)
            return -1;
        peer_now = local_now + magnitude;
    }
    difference = timestamp - (uint32_t)peer_now;
    delta = difference <= INT32_MAX ? (int64_t)difference :
            -(int64_t)(UINT64_C(0x100000000) - difference);
    if (delta >= 0) {
        if (local_now > UINT64_MAX - (uint64_t)delta)
            return -1;
        *local_due = local_now + (uint64_t)delta;
    } else {
        uint64_t past = (uint64_t)-delta;
        *local_due = local_now >= past ? local_now - past : 0;
    }
    return 0;
}
