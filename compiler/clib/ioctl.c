/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <errno.h>
#include <sys/ioctl.h>

int ioctl(int fd, int request, ...)
{
#   warning Implement ioctl()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");

    errno = ENOSYS;
    return -1;
}
