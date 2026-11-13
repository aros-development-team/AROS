/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_RATELIMIT_TYPES_H_
#define _LINUX_RATELIMIT_TYPES_H_

#define DEFAULT_RATELIMIT_INTERVAL  (5 * 100)
#define DEFAULT_RATELIMIT_BURST     10

#define RATELIMIT_MSG_ON_RELEASE    1

struct ratelimit_state {
    int interval;
    int burst;
    int printed;
    int missed;
    unsigned long begin;
    unsigned long flags;
};

#endif
