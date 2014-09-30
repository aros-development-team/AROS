/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>

int f (int i)
{
    if (i <= 1)
	return 1;
    else if (i == 2)
	return 2;

    return f(i-1) + f(i-2);
}

int main (int argc, char ** argv)
{
    int i;

    if (argc == 1)
    {
	printf ("Usage: %s number\n", argv[0]);
	return 5;
    }

    i = atoi (argv[1]);

    printf ("f(%d) = %d\n", i, f(i));

    return 0;
}
