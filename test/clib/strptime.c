/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <time.h>
#include "test.h"
#include <stdio.h>

int main()
{
    struct tm tm;
    TEST((strptime("06:07:08 24.04.1982", "%H:%M:%S %e.%m.%Y", &tm) != NULL));
    TEST((tm.tm_year == 82));
    TEST((tm.tm_mon == 3));
    TEST((tm.tm_mday == 24));
    TEST((tm.tm_hour == 6));
    TEST((tm.tm_min == 7));
    TEST((tm.tm_sec == 8));
    return 0;
}

void cleanup()
{

}
