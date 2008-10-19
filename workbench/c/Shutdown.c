#include <exec/tasks.h>
#include <proto/exec.h>

/**************************************************************************

    NAME
	Shutdown

    SYNOPSIS
	(N/A)

    LOCATION
	Sys:c

    FUNCTION
           Shut down AROS and power off the machine.

**************************************************************************/


int main(void)
{
    ShutdownA(SD_ACTION_POWEROFF);
    return 0;
}
