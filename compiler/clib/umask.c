/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>
#include <sys/stat.h>

#ifndef _CLIB_KERNEL_
static mode_t __umask = S_IWGRP|S_IWOTH;
#endif

mode_t umask(mode_t numask)
{
    GETUSER;

    mode_t oumask = __umask;

    __umask = numask;

    return oumask;
}
