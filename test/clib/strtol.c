#include <stdlib.h>
#include <stdio.h>
#include "test.h"

int main(void)
{
    TEST((strtol("0xff", NULL, 0) == 255UL))
    TEST((strtol("0xff", NULL, 16) == 255UL))
    TEST((strtol("0x0", NULL, 0) == 0UL))
    TEST((strtol("0x0", NULL, 16) == 0UL))
    TEST((strtol("0", NULL, 0) == 0UL))
    TEST((strtol("0", NULL, 16) == 0UL))
    TEST((strtol("0x0 ", NULL, 0) == 0UL))
    TEST((strtol("0x0 ", NULL, 16) == 0UL))
    TEST((strtol("0 ", NULL, 0) == 0UL))
    TEST((strtol("0 ", NULL, 16) == 0UL))
    TEST((strtol("0377", NULL, 0) == 255UL))
    TEST((strtol("255", NULL, 0) == 255UL))
    TEST((strtol("-1", NULL, 0) == -1UL))
    TEST((strtol("-0xff", NULL, 0) == -255UL))
    TEST((strtol("-0xff", NULL, 16) == -255UL))
    TEST((strtol("-ff", NULL, 16) == -255UL))
    TEST((strtol("-0377", NULL, 0) == -255UL))
    TEST((strtol("-377", NULL, 8) == -255UL))
    TEST((strtol("0x7FFFFFFE", NULL, 16) == 0x7FFFFFFE))
    TEST((strtol("0xFFFFFFFE", NULL, 16) == 0xFFFFFFFE))
    return OK;
}

void cleanup(void)
{
}
