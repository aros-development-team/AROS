/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>

#include "test.h"

int main(void)
{
    /* Compiled with -nix */
    /* Expected: will run slave thanks to translating path */

    const char *cmd = "../posix/argv0_slave", *arg1 = "../posix/argv0_slave";
    execl(cmd, cmd, arg1, NULL);

    TEST(0);

    return FAIL;
}

void cleanup(void)
{
    /* NOP */
    return;
}
