/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function symlink().
*/

#include "__errno.h"

#include <unistd.h>

int symlink(const char *oldpath, const char *newpath)

{
    errno = EPERM;
    return -1;
} /* symlink */

