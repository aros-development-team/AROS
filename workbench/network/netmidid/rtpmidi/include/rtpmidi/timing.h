/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef RTPMIDI_TIMING_H
#define RTPMIDI_TIMING_H

#include <stdint.h>

/* Resolve a 32-bit RTP timestamp near the current peer time.
 * Both clocks use the same tick unit; the session supplies the offset. */
int rtpmidi_clock_due(uint32_t timestamp, uint64_t local_now,
                      int64_t peer_to_local, uint64_t *local_due);

#endif
