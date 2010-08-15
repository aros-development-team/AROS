#include <stdlib.h>
#include <unistd.h>

#include "shutdown.h"

#define D(x)

static char **Kernel_ArgV;

void SaveArgs(char **argv)
{
    Kernel_ArgV = argv;
}

void Host_Shutdown(unsigned long action)
{
    switch (action) {
    case SD_ACTION_POWEROFF:
        D(printf("[Shutdown] POWER OFF request\n"));
        exit(0);
    	break;
    case SD_ACTION_COLDREBOOT:
        D(printf("[Shutdown] Cold reboot, dir: %s, name: %s, command line: %s\n", bootstrapdir, bootstrapname, cmdline));
	chdir(bootstrapdir);
	execvp(Kernel_ArgV[0], Kernel_ArgV);

        D(printf("[Shutdown] Unable to re-run AROS\n"));
    }
}
