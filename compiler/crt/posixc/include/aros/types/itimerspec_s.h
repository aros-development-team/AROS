#ifndef _AROS_TYPES_ITIMERSPEC_S_H
#define _AROS_TYPES_ITIMERSPEC_S_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 struct itimerspec definition
*/

#include <aros/types/timespec_s.h>

struct itimerspec
{
    struct timespec	it_interval;	/* timer period */
    struct timespec	it_value;	/* timer expiration */
};

#endif /* _AROS_TYPES_ITIMERSPEC_S_H */
