#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/bptr.h>
#include <dos/dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../rom/exec/etask.h"

static LONG get_default_stack_size()
{
	struct CommandLineInterface *cli = Cli();
	return cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;
}

int main(int argc, char **argv)
{
	struct aros_startup * oldstartup;
	char *fname = "SYS:Utilities/Clock";
	char *full = "";
	int lastresult;
	
	oldstartup = (struct aros_startup *)GetIntETask(FindTask(NULL))->iet_startup;
	
	if(fname) {

		BPTR seglist = LoadSeg(fname);
		if(seglist)
		{
			SetProgramName(fname);
			lastresult=RunCommand(seglist,get_default_stack_size(),
					full,strlen(full));
			UnLoadSeg(seglist);
		}
	}
	
	printf("current iet_startup: %p, old iet_startup: %p\n", (struct aros_startup *)GetIntETask(FindTask(NULL))->iet_startup, oldstartup);
	exit(0);
}