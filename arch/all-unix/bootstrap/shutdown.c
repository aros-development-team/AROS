#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shutdown.h"

#define D(x) 

static char **Kernel_ArgV;

void SaveArgs(char **argv)
{
    Kernel_ArgV = argv;
}

void Host_Shutdown(unsigned char warm)
{
    /* Warm reboot is not implemented yet */
    if (warm)
	return;

    D(printf("[Shutdown] Cold reboot, dir: %s, name: %s\n", bootstrapdir, Kernel_ArgV[0]));
    chdir(bootstrapdir);
    execvp(Kernel_ArgV[0], Kernel_ArgV);

    D(printf("[Shutdown] Unable to re-run AROS\n"));
}
