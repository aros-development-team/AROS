/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <errno.h>

pid_t fork()
{
    #warning Implement fork()
    errno = EPERM;
    return -1;
}
