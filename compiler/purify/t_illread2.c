/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

int main (int argc, char ** argv)
{
    int * x;
    int a, b;

    x = &b;

    b = 0;

    a = *x++;	/* ok */
    a = *x++;	/* illegal: Uninitialized memory */

    x = (int *)&main;

    a = *x; /* illegal: Code read */

    x = (int *)0xDEADBEEF;

    a = *x; /* illegal: Illegal pointer */
}
