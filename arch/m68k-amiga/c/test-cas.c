/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>

int main(int argc, char **argv)
{
    unsigned curval = 0, oldval = 0, newval = 1, result;
    unsigned *mem = &curval;

    printf("Testing CAS instruction...\n");
    
	__asm__ __volatile__("casl %0,%2,%1"
			     : "=d" (result), "=m" (*mem)
			     : "d" (newval), "0" (oldval), "m" (*mem));

    printf("CAS result = %d.\n", curval);

    return 0;
}

