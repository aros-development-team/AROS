/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: Reboot CLI command
    Lang: English              
 */

 /******************************************************************************

    NAME

        Reboot

    SYNOPSIS

        (N/A)

    LOCATION

        Sys:C

    FUNCTION

        It reboots the machine ( performs a cold reboot - Coldreboot() )

    NOTES

        Any programs and data in memory will be lost and all disk
        activity will cease – Make sure no disk access is being 
        carried out by your computer.


    SEE ALSO

	QuitAROS

******************************************************************************/
 
#include <proto/exec.h>

int main() {
    ColdReboot();

    return 0;
}
