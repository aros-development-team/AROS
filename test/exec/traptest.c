/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int a, b, c;
    int do_illegal = 0;
    int *ptr = (unsigned int *)0xABADC0DE;

    if (argc > 1)
    {
    	if (!stricmp("illegal", argv[1]))
    	{
    	    do_illegal = 1;
    	    if (argc > 2)
    	    	ptr = (int *)atol(argv[2]);
    	}
    }

    if (!do_illegal)
    {
    	a = 3;
    	b = 0;
	printf("Attempting to divide by zero...\n");
	c = a/b;
	printf("Done, result is: %d!\n", c);
    }

    printf("Trying illegal access at 0x%p now...\n", ptr);
    a = *ptr;
    printf("Done, result is: %d!\n", a);

    return 0;
}
