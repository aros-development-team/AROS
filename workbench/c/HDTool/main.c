#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "install.h"
#include "list.h"
#include "setup.h"

char *template = "COMMAND/A,ARGS/F";
char *cmdtemplate = "LIST/S,SETUPS/S,INSTALL/S";

LONG doCommand(char *name, STRPTR command, STRPTR args) {
LONG retval = RETURN_FAIL;
IPTR myargs[]={0,0,0,0};
struct RDArgs *rdargs;
struct RDArgs rda = {{command, strlen(command), 0}, 0, 0, 0, NULL, 0};

	rda.RDA_Source.CS_Buffer[rda.RDA_Source.CS_Length]='\n';
	rdargs = ReadArgs(cmdtemplate, myargs, &rda);
	if (rdargs)
	{
		if (myargs[0])
			retval = list(name, args);
		if (myargs[1])
			retval = setup(name, args);
		if (myargs[2])
			retval = install(name, args);
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), name);
	return retval;
}

int main(int argc, char **argv) {
int retval = RETURN_FAIL;
IPTR myargs[]={0,0};
struct RDArgs *rdargs;

	rdargs = ReadArgs(template, myargs, NULL);
	if (rdargs)
	{
		retval = doCommand(argv[0], (STRPTR)myargs[0], (STRPTR)myargs[1]);
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), argv[0]);
	return retval;
}

