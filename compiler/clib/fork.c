/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <unistd.h>
#include <errno.h>

pid_t fork()
{
#   warning Implement fork()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");

    errno = EPERM;
    return -1;
}
