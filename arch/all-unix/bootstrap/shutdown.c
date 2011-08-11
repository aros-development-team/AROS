#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <runtime.h>

#include "shutdown.h"

#define D(x) 

static char **Kernel_ArgV;

void SaveArgs(char **argv)
{
    Kernel_ArgV = argv;
}

void Host_ColdBoot(void)
{
    D(printf("[Bootstrap] Cold reboot, dir: %s, name: %s\n", bootstrapdir, Kernel_ArgV[0]));
    if (chdir(bootstrapdir)==0)
    	execvp(Kernel_ArgV[0], Kernel_ArgV);

    DisplayError("Failed to restart AROS");
}
