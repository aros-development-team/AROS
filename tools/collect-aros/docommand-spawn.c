#include <process.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "docommand.h"
#include "misc.h"

void docommandv(const char *command, char *argv[])
{
    int ret = spawnv(P_WAIT, command, argv);
    if (ret == -1)
    {
	fatal(command, strerror(errno));
    }
    if (ret > 0)
    {
        exit(ret);
    }
}

