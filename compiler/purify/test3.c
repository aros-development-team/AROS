/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

int main (int argc, char ** argv)
{
    char * str;
    int t;

    if (argc == 1)
    {
	printf ("Too few arguments\n");
	return 10;
    }

    str = "Hello world.\n";

    printf ("%s", str);

    printf ("&argc=%p\n", &argc);
    printf ("argv=%p\n", argv);

    for (t=0; t<argc; t++)
	printf ("Arg %d: %s\n", t, argv[t]);

    argc = 1;
    *argv = "test";

    argv[1][1] = 'x'; /* Illegal Write */

    return 0;
}
