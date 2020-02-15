/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include <stdio.h>
#include "test.h"

int main(void)
{
#if defined(AROS_HAVE_LONG_LONG)
    TEST((strtoull("0xff", NULL, 0) == 255ULL))
    TEST((strtoull("0xff", NULL, 16) == 255ULL))
    TEST((strtoull("0x0", NULL, 0) == 0ULL))
    TEST((strtoull("0x0", NULL, 16) == 0ULL))
    TEST((strtoull("0", NULL, 0) == 0ULL))
    TEST((strtoull("0", NULL, 16) == 0ULL))
    TEST((strtoull("0x0 ", NULL, 0) == 0ULL))
    TEST((strtoull("0x0 ", NULL, 16) == 0ULL))
    TEST((strtoull("0 ", NULL, 0) == 0ULL))
    TEST((strtoull("0 ", NULL, 16) == 0ULL))
    TEST((strtoull("0377", NULL, 0) == 255ULL))
    TEST((strtoull("255", NULL, 0) == 255ULL))
    TEST((strtoull("-1", NULL, 0) == -1ULL))
    TEST((strtoull("-0xff", NULL, 0) == -255ULL))
    TEST((strtoull("-0xff", NULL, 16) == -255ULL))
    TEST((strtoull("-ff", NULL, 16) == -255ULL))
    TEST((strtoull("-0377", NULL, 0) == -255ULL))
    TEST((strtoull("-377", NULL, 8) == -255ULL))
#endif
    return OK;
}

void cleanup(void)
{
}
