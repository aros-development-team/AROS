/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <errno.h>
#include <sys/mount.h>

int getfsstat(struct statfs *buf, long bufsize, int flags)
{
#   warning Implement getfsstat()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    errno = ENOSYS;
    return -1;
}
