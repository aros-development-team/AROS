/*
    Copyright  1995-2026, The AROS Development Team. All rights reserved.
 
    Desc: Reboot CLI command
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
    IPTR args[2] = { 0, 0 };

    rda = ReadArgs("COLD/S,WARM/S", args, NULL);

    if (rda == NULL)
    {
        PrintFault(IoErr(),"Reboot");
        return RETURN_FAIL;
    }
    FreeArgs(rda);

    if (args[0])
        ShutdownA(SD_ACTION_COLDREBOOT);
    else if (args[1])
        ColdReboot();
    else
        /* Whatever the platform supports - cold preferred, warm fallback */
        ShutdownA(SD_ACTION_REBOOT);

    /* If we are here, shutdown did not work for some reason */
    Delay(25);
    PutStr("This action is not supported\n");
    return 0;
}
