/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * POSIX.1-2008 header file sys/resource.h
 */

#ifndef SYS_RESOURCE_H
#define SYS_RESOURCE_H

#include <aros/system.h>
#include <aros/cpu.h>

/* NOTIMPL
PRIO_PROCESS
PRIO_PGRP
PRIO_USER
*/

typedef signed AROS_64BIT_TYPE rlim_t;

/* FIXME: Is this value backwards compatible save ? */
#define RLIM_INFINITY (~(rlim_t)0)
/* NOTIMPL
RLIM_SAVED_MAX
RLIM_SAVED_CUR
*/

/* NOTIMPL
RUSAGE_SELF
RUSAGE_CHILDREN
*/

struct rlimit {
    rlim_t rlim_cur; /* The current (soft) limit */
    rlim_t rlim_max; /* The hard limit */
};

#include <aros/types/timeval_s.h>

/* NOTIMPL
struct rusage
*/

#define RLIMIT_CORE     0	/* Limit on size of core file */
#define RLIMIT_CPU      1	/* Limit on CPU time per process */
#define RLIMIT_DATA     2	/* Limit on data segment size */
#define RLIMIT_FSIZE    3	/* Limit on file size */
#define RLIMIT_NOFILE   4	/* Limit on number of open files */
#define RLIMIT_STACK    5	/* Limit on stack size */
#define RLIMIT_AS       6	/* Limit on adress space size */

#include <aros/types/id_t.h>


__BEGIN_DECLS

/* NOTIMPL int getpriority(int, id_t); */
int getrlimit(int, struct rlimit *);
/* NOTIMPL getrusage(int, struct rusage *); */
/* NOTIMPL setpriority(int, id_t, int); */
int setrlimit(int, const struct rlimit *);

__END_DECLS

#endif /* SYS_RESOURCE_H */
