#ifndef _AROS_TYPES_TIMESPEC_S_H
#define _AROS_TYPES_TIMESPEC_S_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosnixc/include/aros/types/timespec_s.h 35162 2010-10-24T13:27:50.962835Z verhaegs  $

    POSIX.1-2008 struct timespec definition
*/

#include <aros/types/time_t.h>

struct timespec
{
    time_t		tv_sec;		/* seconds */
    long		tv_nsec;	/* nanoseconds */
};

#endif /* _AROS_TYPES_TIMESPEC_S_H */
