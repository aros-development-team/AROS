/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>
#include <sys/stat.h>

#warning FIXME: Implement umask() properly
mode_t umask(mode_t numask)
{
    mode_t oumask = __umask;

    __umask = numask;

    return oumask;
}
