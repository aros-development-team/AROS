/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function link().
*/

#include "__errno.h"

#include <unistd.h>

int link(const char *oldpath, const char *newpath)

{
    errno = EPERM;
    return -1;
} /* link */

