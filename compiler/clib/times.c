/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function times().
*/

#include <aros/debug.h>

#include <sys/times.h>

clock_t times(struct tms *buffer)
{
#   warning Implement times()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");

    return (clock_t) -1;
}

