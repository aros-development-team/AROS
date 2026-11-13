#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include "install.h"

char *installtemplate = "CPU/A,ARCH/A,ARGS/F";

LONG install(char *name, STRPTR args) {
BOOL retval = RETURN_FAIL;
IPTR myargs[]={0,0,0};
struct RDArgs *rdargs;
struct RDArgs rda={{args, strlen(args), 0}, 0, 0, 0, NULL, 0};

	rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_Length]='\n';
	rdargs = ReadArgs(installtemplate, myargs, &rda);
	if (rdargs)
	{
		printf
		(
			"install-%s-%s %s\n",
			(STRPTR)myargs[0], (STRPTR)myargs[1], (STRPTR)myargs[2]
		);
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), name);
	return retval;
}


