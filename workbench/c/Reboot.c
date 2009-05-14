/*
    Copyright  1995-2009, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: Reboot CLI command
    Lang: English              
*/

/******************************************************************************

    NAME

        Reboot

    SYNOPSIS

        COLD/S

    LOCATION

        C:

    FUNCTION

        It reboots the machine

    INPUTS

        COLD --  tells to perform cold (complete) reboot of the machine.
        	 Otherwise only AROS is restarted.

    NOTES

        Any programs and data in memory will be lost and all disk
        activity will cease. Make sure no disk access is being 
        carried out by your computer.


    SEE ALSO

	Shutdown

******************************************************************************/

#include <dos/dos.h>
#include <exec/tasks.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

int __nocommandline;

int main()
{
    struct RDArgs *rda;
    IPTR cold = 0;

    rda = ReadArgs("COLD/S", &cold, NULL);

    if (rda == NULL)
    {
	PrintFault(IoErr(),"Reboot");
        return RETURN_FAIL;
    }
    FreeArgs(rda);

    ShowImminentReset();
    if (cold)
        ShutdownA(SD_ACTION_COLDREBOOT);
    else
	ColdReboot();

    /* If we are here, shutdown did not work for some reason */
    PutStr("This action is not supported\n");
    return 0;
}
