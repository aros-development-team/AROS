/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function symlink().
*/

#include <aros/debug.h>

#include <unistd.h>
#include "__errno.h"

int symlink(const char *oldpath, const char *newpath)
{
#   warning Implement symlink()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");

    errno = EPERM;
    return -1;
} /* symlink */

