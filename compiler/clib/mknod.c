/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function mknod().
*/

#include "__errno.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int mknod(const char *pathname, mode_t mode, dev_t dev)

{
    errno = EPERM;
    return -1;
} /* mknod */

