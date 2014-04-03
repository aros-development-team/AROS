/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>

/**************************************************************************

    NAME
	Shutdown

    SYNOPSIS
	(N/A)

    LOCATION
	C:

    FUNCTION
           Shut down AROS and power off the machine.

**************************************************************************/


int main(void)
{
    ShutdownA(SD_ACTION_POWEROFF);
    PutStr("Shutdown failed\n");
    return RETURN_FAIL;
}
