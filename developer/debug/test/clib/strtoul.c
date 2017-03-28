/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include <stdio.h>
#include "test.h"

int main(void)
{
    TEST((strtoul("0xff", NULL, 0) == 255UL))
    TEST((strtoul("0xff", NULL, 16) == 255UL))
    TEST((strtoul("0x0", NULL, 0) == 0UL))
    TEST((strtoul("0x0", NULL, 16) == 0UL))
    TEST((strtoul("0", NULL, 0) == 0UL))
    TEST((strtoul("0", NULL, 16) == 0UL))
    TEST((strtoul("0x0 ", NULL, 0) == 0UL))
    TEST((strtoul("0x0 ", NULL, 16) == 0UL))
    TEST((strtoul("0 ", NULL, 0) == 0UL))
    TEST((strtoul("0 ", NULL, 16) == 0UL))
    TEST((strtoul("0377", NULL, 0) == 255UL))
    TEST((strtoul("255", NULL, 0) == 255UL))
    TEST((strtoul("-1", NULL, 0) == -1UL))
    TEST((strtoul("-0xff", NULL, 0) == -255UL))
    TEST((strtoul("-0xff", NULL, 16) == -255UL))
    TEST((strtoul("-ff", NULL, 16) == -255UL))
    TEST((strtoul("-0377", NULL, 0) == -255UL))
    TEST((strtoul("-377", NULL, 8) == -255UL))
    return OK;
}

void cleanup(void)
{
}
