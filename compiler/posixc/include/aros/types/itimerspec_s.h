#ifndef _AROS_TYPES_ITIMERSPEC_S_H
#define _AROS_TYPES_ITIMERSPEC_S_H

/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id: /aros/branches/ABI_V1/trunk-aroscsplit/AROS/compiler/arosnixc/include/aros/types/itimerspec_s.h 35162 2010-10-24T13:27:50.962835Z verhaegs  $

    POSIX.1-2008 struct itimerspec definition
*/

#include <aros/types/timespec_s.h>

struct itimerspec
{
    struct timespec	it_interval;	/* timer period */
    struct timespec	it_value;	/* timer expiration */
};

#endif /* _AROS_TYPES_ITIMERSPEC_S_H */
