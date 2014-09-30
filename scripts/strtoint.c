/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>

int main (int argc, char ** argv)
{
    if (argc==1)
	puts ("0");
    else
	printf ("%d\n", strtol (argv[1], NULL, 0));

    return 0;
} /* main */
