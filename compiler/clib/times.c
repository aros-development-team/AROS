/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function times().
*/

#include <sys/times.h>

clock_t times(struct tms *buffer)
{
    return (clock_t) -1;
}

