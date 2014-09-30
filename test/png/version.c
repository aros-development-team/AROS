/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <png.h>
#include <stdio.h>

int main(void)
{
    printf("Version %d\n", png_access_version_number());
    return 0;
}
