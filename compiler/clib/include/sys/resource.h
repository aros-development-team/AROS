/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef SYS_RESOURCE_H
#define SYS_RESOURCE_H

#include <sys/types.h>

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

#define RLIMIT_CPU      0
#define RLIMIT_FSIZE    1
#define RLIMIT_DATA     2
#define RLIMIT_STACK    3
#define RLIMIT_CORE     4
#define RLIMIT_RSS      5
#define RLIMIT_NPROC    6
#define RLIMIT_NOFILE   7
#define RLIMIT_MEMLOCK  8
#define RLIMIT_AS       9
#define RLIMIT_LOCKS    10
#define RLIMIT_SIGPENDING       11
#define RLIMIT_MSGQUEUE 12
#define RLIMIT_NICE     12
#define RLIMIT_RTPRIO   14
#define RLIMIT_RTTIME   15
#define RLIMIT_NLIMITS  16

#define RLIM_INFINITY (~(rlim_t)0)

int getrlimit(int resource, struct rlimit *rlp);
int setrlimit(int resource, const struct rlimit *rlp);

#endif /* SYS_RESOURCE_H */
