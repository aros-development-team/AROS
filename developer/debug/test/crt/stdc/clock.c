/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <time.h>
#include <proto/dos.h>

int main(void)
{
    clock_t t1, t2;

    t1 = clock();
    printf("Tick = %ld\n", t1);

    Delay(75);

    t2 = clock();
    printf("Expected: Tick = 75, Secs = 1.500000\n");
    printf("Got     : Tick = %ld, Secs = %f\n", t2, ((float)(t2 - t1))/CLOCKS_PER_SEC);

    return 0;
}
