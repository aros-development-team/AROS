/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>

#include "test.h"

int main(int argc, char *argv[])
{
    TEST(argc == 2);
    TEST(strcmp(argv[0], argv[1]) == 0);

    return OK;
}

void cleanup(void)
{
    /* NOP */
    return;
}
