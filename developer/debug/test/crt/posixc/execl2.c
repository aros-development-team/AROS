/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "test.h"

int main(void)
{
    int fd;
    FILE *f;

    fd = open("execl2_out", O_RDWR|O_CREAT, 00700);
    TEST(fd != -1);

    f = fdopen(fd, "w");
    TEST(f != NULL);

    fputs("OK\n", f);
    fflush(f);

    char arg[10];
    sprintf(arg, "%d", fd);
    execl("execl2_slave", "execl2_slave", arg, NULL);
    TEST( 0 ); /* Should not be reached */

    exit(20);
}

void cleanup(void)
{
    /* NOP */
    return;
}
