/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: stub function generator for clib functions
    Lang: english
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <aros/cpu.h>
#include <exec/libraries.h>

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
	    printf(STUBCODE,
	           syscalls[n].name, "aroscbase",
	           &(__AROS_GETJUMPVEC(NULL, (n+1+LIB_RESERVED))->vec));
	    printf("\n");
	    if (syscalls[n].alias[0] != '\0' )
	    {
	        printf(".globl %s\n"
		       "\t.set %s, %s\n",
		       syscalls[n].alias, syscalls[n].alias, syscalls[n].name);
	    }

	    return 0;
	}

    fprintf(stderr, "Invalid function name \"%s\"\n", argv[1]);

    return 1;
}
