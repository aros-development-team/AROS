/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#include <proto/utility.h>

int main(int argc, char **argv)
{
    QUAD val;

    val = SMult64(0x12345678,0xdeadcafe);
    printf("0x12345678 * 0xdeadcafe = 0x%llx\n", (unsigned long long)val);

    return 0;
}
