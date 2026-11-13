#include <string.h>
#include <stdio.h>
#include <proto/dos.h>
#include "setup.h"

char *setuptemplate = "DEVICE/K/A,UNIT/K/N/A,PARTITION/K,SCRIPT/K";

LONG setup(char *name, STRPTR args) {
BOOL retval = RETURN_FAIL;
IPTR myargs[]={0,0,0,0};
struct RDArgs *rdargs;
struct RDArgs rda = {{args, strlen(args), 0}, 0, 0, 0, NULL, 0};

	rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_Length]='\n';
	rdargs = ReadArgs(setuptemplate,myargs, &rda);
	if (rdargs)
	{
		printf
		(
			"setup\n\tdevice=%s unit=%ld\n",
			(STRPTR)myargs[0], (long)*(LONG *)myargs[1]
		);
		if (myargs[2])
		{
			printf("\ttable in partition=%s\n", (STRPTR)myargs[2]);
		}
		else
			printf("\ttable in whole HD\n");
		if (myargs[3])
		{
			printf("\tscipt is %s\n", (STRPTR)myargs[3]);
		}
		else
			printf("\tinteractive\n");
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), name);
	return retval;
}

