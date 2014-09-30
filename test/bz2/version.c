/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <bzlib.h>
#include <stdio.h>

int main(void)
{
    printf("Version %s\n", BZ2_bzlibVersion());
    return 0;
}
