/*
    Copyright 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: stub function generator for clib functions
    Lang: english
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <aros/machine.h>
#include <exec/libraries.h>

#define SYSTEM_CALL(name) "" #name "" ,

static char *names[]=
{
#include <sys/syscall.def>
NULL
};

int main(int argc, char *argv[])
{
    int n;

    if (argc != 2)
    {
    	fprintf(stderr, "Argument required: either '-list' or the name of the function\n");
	return 1;
    }

    if (!strcmp(argv[1], "-list"))
    {
 	for (n=0; names[n]; n++)
    	    printf("%s\n", names[n]);

	return 0;
    }

    for (n=0; names[n]; n++)
	if (!strcmp(names[n], argv[1]))
	{
	    printf(STUBCODE,
	           names[n], "aroscbase",
	           &(__AROS_GETJUMPVEC(0, n+1+LIB_RESERVED)->vec));
	    printf("\n");

	    return 0;
	}

    fprintf(stderr, "Invalid function name \"%s\"\n", argv[1]);

    return 1;
}
