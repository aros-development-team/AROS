/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

pid_t wait(int *status)
{
    #warning Implement wait()
    errno = EPERM;
    return -1;
}
