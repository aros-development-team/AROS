/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: stub function generator for clib functions
    Lang: english
*/

#include "archspecific.h"

#undef __const

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SYSTEM_CALL(name, alias...) { #name , #alias }, 

struct
{
    char *name;
    char *alias;
} syscalls [] =
{
    #include "../include/sys/syscall.def"
    { NULL, NULL }
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
 	for (n=0; syscalls[n].name; n++)
    	    printf("%s\n",syscalls[n].name);

	return 0;
    }

    for (n=0; syscalls[n].name != NULL; n++)
	if (!strcmp(syscalls[n].name, argv[1]))
	{
	    printf(STUBCODE_INIT);
	    printf(STUBCODE,
	           syscalls[n].name, "aroscbase",
	           JUMPVEC(n)
	    );
	    if (syscalls[n].alias[0] != '\0' )
	    {
	        printf(ALIASCODE,
		       syscalls[n].name, syscalls[n].alias
		);
	    }

	    return 0;
	}

    fprintf(stderr, "Invalid function name \"%s\"\n", argv[1]);

    return 1;
}
