/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <zlib.h>
#include <stdio.h>

int main(void)
{
    printf("Version %s\n", zlibVersion());
    return 0;
}
