#include <stdlib.h>

#include "shutdown.h"

#define D(x)

void SaveArgs(char **argv)
{
    /* Nothing to save since we can't re-run ourselves */
}

void Host_Shutdown(unsigned long action)
{
    if (action == SD_ACTION_POWEROFF) {
        D(printf("[Shutdown] POWER OFF request\n"));
        exit(0);
    }
}
