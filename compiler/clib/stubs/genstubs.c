/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: stub function generator for clib functions
    Lang: english
*/

/* LVOs reserved by exec */
#define LIB_RESERVED 4

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

    if (strcmp(argv[1], "-list") == 0)
    {
 	for (n=0; syscalls[n].name; n++)
    	    printf("%s\n",syscalls[n].name);

	return 0;
    }

    for (n=0; syscalls[n].name != NULL; n++)
	if (strcmp(syscalls[n].name, argv[1]) == 0)
	{
	    printf("#include <aros/cpu.h>\n"
		   "\n"
		   "AROS_LIBFUNCSTUB(%s, aroscbase, %d)\n",
		   syscalls[n].name, n+1+LIB_RESERVED
	    );

	    return 0;
	}

    fprintf(stderr, "Invalid function name \"%s\"\n", argv[1]);

    return 1;
}
