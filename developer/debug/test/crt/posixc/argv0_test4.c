/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>

#include "test.h"

int main(void)
{
    const char *cmd = "../posix/argv0_slave", *arg1 = "../posix/argv0_slave"; 
    execl(cmd, cmd, arg1, NULL);

    return FAIL;
}
