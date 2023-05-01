#ifndef _POSIXC_SYS_TIMES_H
#define _POSIXC_SYS_TIMES_H

/*
    Copyright © 2004-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file sys/times.h
*/

#include <aros/types/clock_t.h>

struct tms 
{
    clock_t tms_utime;  /* User time */
    clock_t tms_stime;  /* System time */
    clock_t tms_cutime; /* User time of children */
    clock_t tms_cstime; /* System time of children */
};

__BEGIN_DECLS

clock_t times(struct tms *buffer);

__END_DECLS

#endif /* _POSIXC_SYS_TIMES_H */

