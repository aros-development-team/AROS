/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

int main(int argc, const char *argv[])
{
    int fd;
    FILE *f;

    sscanf(argv[1], "%d", &fd);
    f = fdopen(fd, "r");
    TEST(f != NULL);
    
    char line[10];
    fseek(f, 0, SEEK_SET);
    TEST(fgets(line, 10, f) != NULL);

    TEST(strcmp(line, "OK\n") == 0);

    return 0;
}

void cleanup(void)
{
    /* NOP */
    return;
}
